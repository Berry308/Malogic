#include "Magic/MagicCircleManagerComponent.h"

#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Magic/MalogicMagicCircleDefinition.h"
#include "Magic/MalogicMagicCircleInstance.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MagicCircleManagerComponent)

UMagicCircleManagerComponent::UMagicCircleManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UMagicCircleManagerComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] must be attached to a Pawn."), *GetNameSafe(this));
		return;
	}

	GetPawn<APawn>()->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::HandlePawnControllerChanged);
	BindPawnExtensionDelegates();
}

void UMagicCircleManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	BindPawnExtensionDelegates();
	RequestLocalStateSync();

	if (HasAuthority())
	{
		ApplyAbilitySetOnServer(EquippedMagicCircle);
	}
}

void UMagicCircleManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* Pawn = GetPawn<APawn>())
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandlePawnControllerChanged);
	}

	RemoveInputMappingForLocalPlayer();
	Super::EndPlay(EndPlayReason);
}

void UMagicCircleManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	BindPawnExtensionDelegates();
	RequestLocalStateSync();

	if (HasAuthority())
	{
		ApplyAbilitySetOnServer(EquippedMagicCircle);
	}
}

void UMagicCircleManagerComponent::UninitializeComponent()
{
	RemoveInputMappingForLocalPlayer();

	if (HasAuthority())
	{
		CancelActiveAbilitiesFromDefinition(EquippedMagicCircle
			? EquippedMagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>()
			: nullptr);
		RemoveAbilitySetOnServer();
		EquippedMagicCircle = nullptr;
		RequestLocalStateSync();
	}

	Super::UninitializeComponent();
}

void UMagicCircleManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bLocalStateSyncPending && TrySynchronizeLocalState())
	{
		CompleteLocalStateSync();
	}
}

void UMagicCircleManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquippedMagicCircle);
}

bool UMagicCircleManagerComponent::EquipMagicCircle(TSubclassOf<UMalogicMagicCircleDefinition> NewMagicCircle)
{
	if (!NewMagicCircle)
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleManagerComponent [%s] rejected an invalid magic circle definition."), *GetNameSafe(this));
		return false;
	}

	if (EquippedMagicCircle == NewMagicCircle)
	{
		return false;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->HasAuthority())
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleManagerComponent [%s] rejected EquipMagicCircle because the owner is not authoritative."), *GetNameSafe(this));
		return false;
	}

	const UMalogicMagicCircleDefinition* NewDefinition = NewMagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>();
	if (!NewDefinition)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] could not get the CDO for definition [%s]."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle));
		return false;
	}

	bool bDefinitionIsValid = true;

#pragma region Validation
	if (!NewDefinition->MagicCircleToSpawn)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because MagicCircleToSpawn is not configured."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle));
		bDefinitionIsValid = false;
	}
	

	if (!NewDefinition->AbilitySetForPlayer)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because AbilitySetForPlayer is not configured."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle));
		bDefinitionIsValid = false;
	}

	if (!NewDefinition->AbilitySetForMagicCircle)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because AbilitySetForMagicCircle is not configured."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle));
		bDefinitionIsValid = false;
	}

	if (!FMath::IsFinite(NewDefinition->BaseBuildingTime) || NewDefinition->BaseBuildingTime < 0.0f)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because BaseBuildingTime [%f] is invalid."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle), NewDefinition->BaseBuildingTime);
		bDefinitionIsValid = false;
	}

	if (!FMath::IsFinite(NewDefinition->BaseMaxDeployDistance) || NewDefinition->BaseMaxDeployDistance < 0.0f)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because BaseMaxDeployDistance [%f] is invalid."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle), NewDefinition->BaseMaxDeployDistance);
		bDefinitionIsValid = false;
	}

	if (!FMath::IsFinite(NewDefinition->MaxShootDistance) || NewDefinition->MaxShootDistance <= 0.0f)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because MaxShootDistance [%f] is invalid."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle), NewDefinition->MaxShootDistance);
		bDefinitionIsValid = false;
	}

	if (!FMath::IsFinite(NewDefinition->BeamRadius) || NewDefinition->BeamRadius <= 0.0f)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because BeamRadius [%f] is invalid."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle), NewDefinition->BeamRadius);
		bDefinitionIsValid = false;
	}

	if (NewDefinition->bIsPreDeploy && !NewDefinition->PreviewActor)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] rejected definition [%s] because bIsPreDeploy is enabled but PreviewActor is not configured."), *GetNameSafe(this), *GetNameSafe(NewMagicCircle));
		bDefinitionIsValid = false;
	}
#pragma endregion

	if (!bDefinitionIsValid)
	{
		return false;
	}

	const TSubclassOf<UMalogicMagicCircleDefinition> PreviousMagicCircle = EquippedMagicCircle;
	if (PreviousMagicCircle)
	{
		CancelActiveAbilitiesFromDefinition(PreviousMagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>());
	}
	RemoveInputMappingForLocalPlayer();
	RemoveAbilitySetOnServer();

	EquippedMagicCircle = NewMagicCircle;
	ApplyAbilitySetOnServer(NewMagicCircle);

	// RepNotify is not called automatically on the server. Keep a listen-server's local state in sync.
	OnRep_EquippedMagicCircle(PreviousMagicCircle);
	Pawn->ForceNetUpdate();
	return true;
}

void UMagicCircleManagerComponent::UnequipMagicCircle()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->HasAuthority())
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleManagerComponent [%s] rejected UnequipMagicCircle because the owner is not authoritative."), *GetNameSafe(this));
		return;
	}

	const TSubclassOf<UMalogicMagicCircleDefinition> PreviousMagicCircle = EquippedMagicCircle;
	if (!PreviousMagicCircle)
	{
		RemoveInputMappingForLocalPlayer();
		RemoveAbilitySetOnServer();
		RequestLocalStateSync();
		return;
	}

	CancelActiveAbilitiesFromDefinition(PreviousMagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>());
	RemoveInputMappingForLocalPlayer();
	RemoveAbilitySetOnServer();
	EquippedMagicCircle = nullptr;

	OnRep_EquippedMagicCircle(PreviousMagicCircle);
	Pawn->ForceNetUpdate();
}

void UMagicCircleManagerComponent::ApplyAbilitySetOnServer(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircle)
{
	if (!HasAuthority() || bAbilitySetApplied || !MagicCircle)
	{
		return;
	}

	UMalogicAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (!AbilitySystemComponent || AbilitySystemComponent->GetAvatarActor() != GetOwner())
	{
		return;
	}

	const UMalogicMagicCircleDefinition* Definition = MagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>();
	if (!Definition || !Definition->AbilitySetForPlayer)
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleManagerComponent [%s] cannot equip definition [%s] without AbilitySetForPlayer."), *GetNameSafe(this), *GetNameSafe(MagicCircle));
		return;
	}

	Definition->AbilitySetForPlayer->GiveToAbilitySystem(
		AbilitySystemComponent,
		&EquippedAbilitySetHandles,
		const_cast<UMalogicMagicCircleDefinition*>(Definition));
	bAbilitySetApplied = true;
}

void UMagicCircleManagerComponent::RemoveAbilitySetOnServer()
{
	if (!HasAuthority())
	{
		return;
	}

	if (bAbilitySetApplied)
	{
		if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent())
		{
			EquippedAbilitySetHandles.TakeFromAbilitySystem(AbilitySystemComponent);
		}
		else
		{
			UE_LOG(LogMalogic, Warning, TEXT("MagicCircleManagerComponent [%s] could not remove its AbilitySet because the owner ASC is unavailable."), *GetNameSafe(this));
		}
	}

	bAbilitySetApplied = false;
}

void UMagicCircleManagerComponent::ApplyInputMappingForLocalPlayer(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircle)
{
	if (!MagicCircle || !IsLocalInputReady())
	{
		return;
	}

	const UMalogicMagicCircleDefinition* Definition = MagicCircle->GetDefaultObject<UMalogicMagicCircleDefinition>();
	if (!Definition || Definition->DeploymentInputMapping.InputMapping.IsNull())
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	APlayerController* PlayerController = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputMappingContext* MappingContext = Definition->DeploymentInputMapping.InputMapping.LoadSynchronous();
	if (!InputSubsystem || !MappingContext)
	{
		return;
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = false;
	if (Definition->DeploymentInputMapping.bRegisterWithSettings)
	{
		if (UEnhancedInputUserSettings* Settings = InputSubsystem->GetUserSettings())
		{
			Settings->RegisterInputMappingContext(MappingContext);
		}
	}
	InputSubsystem->AddMappingContext(MappingContext, Definition->DeploymentInputMapping.Priority, Options);
	AppliedInputMapping = Definition->DeploymentInputMapping;
	AppliedInputSubsystem = InputSubsystem;
	bHasAppliedInputMapping = true;
}

void UMagicCircleManagerComponent::RemoveInputMappingForLocalPlayer()
{
	if (!bHasAppliedInputMapping)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = AppliedInputSubsystem.Get();
	UInputMappingContext* MappingContext = AppliedInputMapping.InputMapping.LoadSynchronous();
	if (!InputSubsystem || !MappingContext)
	{
		AppliedInputMapping = FInputMappingContextAndPriority();
		AppliedInputSubsystem = nullptr;
		bHasAppliedInputMapping = false;
		return;
	}

	InputSubsystem->RemoveMappingContext(MappingContext);
	AppliedInputMapping = FInputMappingContextAndPriority();
	AppliedInputSubsystem = nullptr;
	bHasAppliedInputMapping = false;
}

void UMagicCircleManagerComponent::OnRep_EquippedMagicCircle(TSubclassOf<UMalogicMagicCircleDefinition> PreviousMagicCircle)
{
	(void)PreviousMagicCircle;
	RequestLocalStateSync();
}

void UMagicCircleManagerComponent::BindPawnExtensionDelegates()
{
	if (bPawnExtensionDelegatesBound)
	{
		return;
	}

	if (UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
	{
		PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemInitialized));
		PawnExtension->OnAbilitySystemUninitialized_Register(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::HandleAbilitySystemUninitialized));
		PawnExtension->OnPawnInputComponentReady_RegisterAndCall(
			FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::RequestLocalStateSync));
		bPawnExtensionDelegatesBound = true;
	}
}

void UMagicCircleManagerComponent::HandleAbilitySystemInitialized()
{
	if (HasAuthority())
	{
		ApplyAbilitySetOnServer(EquippedMagicCircle);
	}

	RequestLocalStateSync();
}

void UMagicCircleManagerComponent::HandleAbilitySystemUninitialized()
{
	RemoveInputMappingForLocalPlayer();

	if (HasAuthority())
	{
		RemoveAbilitySetOnServer();
	}
}

void UMagicCircleManagerComponent::HandlePawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	(void)Pawn;
	(void)OldController;
	(void)NewController;

	RemoveInputMappingForLocalPlayer();
	RequestLocalStateSync();
}

UMalogicAbilitySystemComponent* UMagicCircleManagerComponent::GetOwnerAbilitySystemComponent() const
{
	const UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	return PawnExtension ? PawnExtension->GetMalogicAbilitySystemComponent() : nullptr;
}

//这个函数用于在卸载旧魔法阵前，显式取消该 Definition 授予且仍在运行的玩家能力，主要是部署、瞄准等能力。
void UMagicCircleManagerComponent::CancelActiveAbilitiesFromDefinition(const UMalogicMagicCircleDefinition* Definition)
{
	if (!HasAuthority() || !Definition)
	{
		return;
	}

	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
		AbilitySystemComponent && AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		AbilitySystemComponent->CancelAbilitiesByFunc(
			[AbilitySystemComponent, Definition](const UMalogicGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle)
			{
				(void)Ability;
				const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
				return Spec && Spec->SourceObject.Get() == Definition;
			},
			true);
	}
}

//同步本地状态，确保客户端的输入映射与服务器的装备状态一致。
void UMagicCircleManagerComponent::RequestLocalStateSync()
{
	bLocalStateSyncPending = true;
	bDefinitionNotificationPending = true;
	SetComponentTickEnabled(true);

	if (TrySynchronizeLocalState())
	{
		CompleteLocalStateSync();
	}
}

void UMagicCircleManagerComponent::CompleteLocalStateSync()
{
	bLocalStateSyncPending = false;
	SetComponentTickEnabled(false);

	if (bDefinitionNotificationPending)
	{
		bDefinitionNotificationPending = false;
		OnMagicCircleDefinitionChanged.Broadcast(EquippedMagicCircle);
	}
}

bool UMagicCircleManagerComponent::TrySynchronizeLocalState()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		RemoveInputMappingForLocalPlayer();
		return true;
	}

	if (!Pawn->IsLocallyControlled())
	{
		RemoveInputMappingForLocalPlayer();
		return true;
	}

	if (!IsLocalInputReady())
	{
		return false;
	}

	RemoveInputMappingForLocalPlayer();
	ApplyInputMappingForLocalPlayer(EquippedMagicCircle);
	return true;
}

bool UMagicCircleManagerComponent::IsLocalInputReady() const
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return false;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	return PlayerController && PlayerController->IsLocalController() && PlayerController->GetLocalPlayer() != nullptr;
}
