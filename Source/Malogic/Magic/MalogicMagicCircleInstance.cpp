#include "Magic/MalogicMagicCircleInstance.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "AbilitySystem/Attributes/MalogicCombatSet.h"
#include "AbilitySystem/Attributes/MalogicHealthSet.h"
#include "Character/MalogicHealthComponent.h"
#include "AbilitySystem/TargetData/MalogicGATargetData_MagicCircleSpawnInfo.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Magic/MalogicMagicCircleDefinition.h"
#include "MalogicGameplayTags.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/MagicWeaponStateComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicMagicCircleInstance)

AMalogicMagicCircleInstance::AMalogicMagicCircleInstance()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(false);//在可以动态移动的魔法阵中，此项需要设置为true

	AbilitySystemComponent = CreateDefaultSubobject<UMalogicAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	CombatSet = CreateDefaultSubobject<UMalogicCombatSet>(TEXT("CombatSet"));

	HealthComponent = CreateDefaultSubobject<UMalogicHealthComponent>(TEXT("HealthComponent"));

	SetNetUpdateFrequency(100.0f);
}

void AMalogicMagicCircleInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, ActualBuildingTime);
	DOREPLIFETIME_CONDITION(ThisClass, PredictionId, COND_OwnerOnly);
	DOREPLIFETIME(ThisClass, MagicCircleState);
	DOREPLIFETIME(ThisClass, MagicCircleDefinitionClass);
	DOREPLIFETIME(ThisClass, DeploymentInstigator);
}

//通常在部署创建实例时，由部署者调用，传入魔法阵定义和部署者信息
void AMalogicMagicCircleInstance::InitializeFromDefinition(const UMalogicMagicCircleDefinition* Definition, AActor* InInstigator, float InActualBuildingTime)
{
	if (!HasAuthority())
	{
		UE_LOG(LogMalogic, Warning, TEXT("InitializeFromDefinition called on a non-authority magic circle [%s]."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(Definition))
	{
		UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] was initialized without a valid definition."), *GetNameSafe(this));
		return;
	}

	MagicCircleDefinitionClass = Definition->GetClass();
	DeploymentInstigator = InInstigator;
	// The deploy ability supplies the server-authoritative building time here.
	ActualBuildingTime = FMath::Max(0.0f, InActualBuildingTime);
	BaseBuildingTime = Definition->BaseBuildingTime;
	MagicCircleState = EMagicCircleState::Spawned;
}

void AMalogicMagicCircleInstance::InitializeFromTargetData(FMalogicGATargetData_MagicCircleSpawnInfo& SpawnInfo, uint16 InPredictionId)
{
	if (!HasAuthority())
	{
		UE_LOG(LogMalogic, Warning, TEXT("InitializeFromTargetData called on a non-authority magic circle [%s]."), *GetNameSafe(this));
		return;
	}

	PredictionId = static_cast<int32>(InPredictionId);

	float ElapsedTime = 0.0f;
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState || !FMath::IsFinite(SpawnInfo.ClientSpawnTime))
	{
		return;
	}

	const float ServerWorldTime = GameState->GetServerWorldTimeSeconds();
	if (!FMath::IsFinite(ServerWorldTime))
	{
		return;
	}
	//在此处计算从客户端发送到服务端的时间差，限制最大值为100ms
	ElapsedTime = FMath::Clamp(ServerWorldTime - SpawnInfo.ClientSpawnTime, 0.0f, 0.1f);
	ActualBuildingTime = FMath::Max(0.0f, ActualBuildingTime - ElapsedTime);
}

void AMalogicMagicCircleInstance::BeginPlay()
{
	Super::BeginPlay();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		const UMalogicMagicCircleDefinition* Definition = MagicCircleDefinitionClass
			? MagicCircleDefinitionClass->GetDefaultObject<UMalogicMagicCircleDefinition>()
			: nullptr;

		if (!Definition)
		{
			UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] has no valid definition class."), *GetNameSafe(this));
			HandleOutOfHealth();
			return;
		}

		if (Definition->AbilitySetForMagicCircle)
		{
			Definition->AbilitySetForMagicCircle->GiveToAbilitySystem(AbilitySystemComponent, &GrantedHandles, const_cast<UMalogicMagicCircleDefinition*>(Definition));
			bAbilitySetGranted = true;
		}
		else
		{
			UE_LOG(LogMalogic, Error, TEXT("Magic circle definition [%s] has no AbilitySetForMagicCircle."), *GetNameSafe(Definition));
		}

		//为什么不默认创建HealthSet和CombatSet？因为有些魔法阵可能不需要生命值和战斗属性，所以这些属性是可选的。
		HealthSet = AbilitySystemComponent->GetSet<UMalogicHealthSet>();
		CombatSet = AbilitySystemComponent->GetSet<UMalogicCombatSet>();
		if (HealthSet)
		{
			HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealthEvent);
		}
		else
		{
			UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] has no UMalogicHealthSet after AbilitySet initialization."), *GetNameSafe(this));
		}
		if (!CombatSet)
		{
			UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] has no UMalogicCombatSet after AbilitySet initialization."), *GetNameSafe(this));
		}

		InitializeLifetime();
	}

	if (HealthComponent)
	{
		HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	}

	if (HasAuthority())
	{
		StartBuilding();
	}

	TryInitializeClientBuildingPresentation();
}

void AMalogicMagicCircleInstance::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// After handoff, the client owns only this visual clock; gameplay state remains server-authoritative.
	ClientPresentationElapsedTime += DeltaSeconds;
	const float PresentationAlpha = ActualBuildingTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(ClientPresentationElapsedTime / ActualBuildingTime, 0.0f, 1.0f)
		: 1.0f;
	CurrentBuildingProgress = FMath::Lerp(ClientPresentationStartProgress, 1.0f, PresentationAlpha);
	K2_ApplyBuildingProgress(CurrentBuildingProgress);

	if (CurrentBuildingProgress >= 1.0f)
	{
		SetActorTickEnabled(false);
	}
}

//根据生命周期策略，初始化魔法阵的生命周期管理
void AMalogicMagicCircleInstance::InitializeLifetime()
{
	if (!HasAuthority())
	{
		return;
	}

	if (LifetimeStrategy == EMagicCircleLifetimeStrategy::OnceAfterSomeGA)
	{
		if (!FinishAbilityClass)
		{
			UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] uses OnceAfterSomeGA but FinishAbilityClass is not configured."), *GetNameSafe(this));
			return;
		}

		AbilitySystemComponent->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityFinished);
	}
	else if (LifetimeStrategy == EMagicCircleLifetimeStrategy::PersistentTilDie)
	{
		if(FinishAbilityClass) UE_LOG(LogMalogic, Error, TEXT("Magic circle [%s] uses PersistentTilDie but has FinishAbilityClass [%s]; ignoring the finish ability binding."), *GetNameSafe(this), *GetNameSafe(FinishAbilityClass));
	}
}

void AMalogicMagicCircleInstance::StartBuilding()
{
	if (!HasAuthority() || MagicCircleState != EMagicCircleState::Spawned)
	{
		return;
	}

	SetMagicCircleState(EMagicCircleState::Building);
}

void AMalogicMagicCircleInstance::HandleBuildingFinished()
{
	K2_OnBuildingFinished();

	if (!HasAuthority() || MagicCircleState != EMagicCircleState::Building)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(BuildingTimerHandle);

	SetMagicCircleState(EMagicCircleState::Ready);
}

void AMalogicMagicCircleInstance::HandleMagicCircleReady()
{
	K2_OnMagicCircleReady();

	if (HasAuthority())
	{
		StartLifeTimeTimer();
		ActivateAbilitiesByTag(MalogicGameplayTags::MagicCircle_Ability_BuildFinished);

		switch (ActivateStrategy)
		{
		case EMagicCircleActivateStrategy::Auto:
			ActivateMagic(MalogicGameplayTags::MagicCircle_Ability_Activate);
			break;
		case EMagicCircleActivateStrategy::Manual:
		case EMagicCircleActivateStrategy::Detection:
			break;
		}
	}
}

void AMalogicMagicCircleInstance::HandleMagicCircleFinished()
{
	K2_OnMagicCircleFinished();

	//如果魔法的生命周期策略是OnceAfterSomeGA，那么在Finished状态下，魔法阵应该立即进入Destroyed状态
	if (HasAuthority())
	{
		if (LifetimeStrategy == EMagicCircleLifetimeStrategy::OnceAfterSomeGA)
		{
			SetMagicCircleState(EMagicCircleState::Destroyed);
		}
	}
}

void AMalogicMagicCircleInstance::HandleMagicCircleDestroyed()
{
	K2_OnMagicCircleDestroyed();

	if (HasAuthority())
	{
		ActivateAbilitiesByTag(MalogicGameplayTags::MagicCircle_Ability_Destroyed);
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->CancelAllAbilities();
		}
		if (bAbilitySetGranted)
		{
			GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
			bAbilitySetGranted = false;
		}
		//在此处延迟销毁魔法阵实例，以确保客户端有足够的时间接收状态更新（主要是MagicCircleState）和播放相关的特效或动画
		SetLifeSpan(0.5f);
	}
}

void AMalogicMagicCircleInstance::OnRep_MagicCircleState(EMagicCircleState OldState)
{
	OnMagicCircleStateChanged(OldState, MagicCircleState);
	TryInitializeClientBuildingPresentation();
}

void AMalogicMagicCircleInstance::OnRep_ActualBuildingTime()
{
	TryInitializeClientBuildingPresentation();
}

void AMalogicMagicCircleInstance::OnRep_PredictionId()
{
	TryInitializeClientBuildingPresentation();
}

void AMalogicMagicCircleInstance::OnRep_Owner()
{
	Super::OnRep_Owner();
	TryInitializeClientBuildingPresentation();
}

void AMalogicMagicCircleInstance::TryInitializeClientBuildingPresentation()
{
	if (!HasActorBegunPlay() || GetNetMode() == NM_DedicatedServer || bClientBuildingPresentationInitialized ||
		MagicCircleState == EMagicCircleState::Spawned)
	{
		return;
	}

	float InitialProgress = 0.0f;
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		if (PredictionId == INDEX_NONE)
		{
			return;
		}

		AController* Controller = OwnerPawn->GetController();
		UMagicWeaponStateComponent* WeaponStateComponent = Controller
			? Controller->FindComponentByClass<UMagicWeaponStateComponent>()
			: nullptr;
		if (WeaponStateComponent)
		{
			// PredictionId pairs this replicated instance with the local predicted view actor.
			WeaponStateComponent->ConsumePredictiveViewActor(static_cast<uint16>(PredictionId), InitialProgress);
		}
	}

	StartClientBuildingPresentation(MagicCircleState == EMagicCircleState::Building ? InitialProgress : 1.0f);
}

void AMalogicMagicCircleInstance::StartClientBuildingPresentation(float InitialProgress)
{
	bClientBuildingPresentationInitialized = true;
	ClientPresentationElapsedTime = 0.0f;
	ClientPresentationStartProgress = ActualBuildingTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(InitialProgress, 0.0f, 1.0f)
		: 1.0f;
	CurrentBuildingProgress = ClientPresentationStartProgress;
	K2_ApplyBuildingProgress(CurrentBuildingProgress);
	SetActorTickEnabled(CurrentBuildingProgress < 1.0f && ActualBuildingTime > KINDA_SMALL_NUMBER);
}

void AMalogicMagicCircleInstance::SetMagicCircleState(EMagicCircleState NewState)
{
	if (!HasAuthority() || MagicCircleState == NewState)
	{
		return;
	}

	const EMagicCircleState OldState = MagicCircleState;
	MagicCircleState = NewState;
	ForceNetUpdate();
	OnMagicCircleStateChanged(OldState, NewState);
}

//此函数后续可能需要修改，针对不同的OldState到NewState组合做不同的处理
void AMalogicMagicCircleInstance::OnMagicCircleStateChanged(EMagicCircleState OldState, EMagicCircleState NewState)
{
	switch (NewState)
	{
	case EMagicCircleState::Building:
		if (HasAuthority())
		{
			GetWorldTimerManager().ClearTimer(BuildingTimerHandle);
			if (ActualBuildingTime <= 0.0f)
			{
				GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::HandleBuildingFinished);
				UE_LOG(LogMalogic, Warning, TEXT("Magic circle [%s] has no remaining building time. ActualBuildingTime [%f]."), *GetNameSafe(this), ActualBuildingTime);
			}
			else
			{
				GetWorldTimerManager().SetTimer(BuildingTimerHandle, this, &ThisClass::HandleBuildingFinished, ActualBuildingTime, false);
			}
		}
		break;

	case EMagicCircleState::Ready:
		HandleMagicCircleReady();
		break;

	case EMagicCircleState::Active:
		break;

	case EMagicCircleState::Finished:
		GetWorldTimerManager().ClearTimer(LifeTimeTimerHandle);
		HandleMagicCircleFinished();
		break;

	case EMagicCircleState::Destroyed:
		HandleMagicCircleDestroyed();
		break;

	default:
		break;
	}
}

bool AMalogicMagicCircleInstance::ActivateAbilitiesByTag(const FGameplayTag& ActivationTag)
{
	if (!HasAuthority() || !ActivationTag.IsValid() || !AbilitySystemComponent)
	{
		return false;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToActivate;
	{
		FScopedAbilityListLock AbilityListLock(*AbilitySystemComponent);
		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (AbilitySpec.Ability && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(ActivationTag))
			{
				HandlesToActivate.Add(AbilitySpec.Handle);
			}
		}
	}

	bool bActivatedAny = false;
	for (const FGameplayAbilitySpecHandle& Handle : HandlesToActivate)
	{
		bActivatedAny |= AbilitySystemComponent->TryActivateAbility(Handle);
	}
	return bActivatedAny;
}

void AMalogicMagicCircleInstance::ActivateMagic(const FGameplayTag& ActivationTag)
{
	if (!HasAuthority() || MagicCircleState != EMagicCircleState::Ready)
	{
		return;
	}

	SetMagicCircleState(EMagicCircleState::Active);

	if (!ActivateAbilitiesByTag(ActivationTag))
	{
		UE_LOG(LogMalogic, Warning, TEXT("Magic circle [%s] could not activate any ability for tag [%s]."), *GetNameSafe(this), *ActivationTag.ToString());
		HandleActivateMagicFail(ActivationTag);
	}
}

void AMalogicMagicCircleInstance::HandleActivateMagicFail(const FGameplayTag& ActivationTag)
{
	// The base behavior treats an activation failure as a completed, one-shot magic circle.
	(void)ActivationTag;
	FinishMagicCircle();
}

void AMalogicMagicCircleInstance::StartLifeTimeTimer()
{
	if (!HasAuthority() || LifeTime <= 0.0f || MagicCircleState != EMagicCircleState::Ready)
	{
		return;
	}

	bLifeTimeExpired = false;
	GetWorldTimerManager().ClearTimer(LifeTimeTimerHandle);
	GetWorldTimerManager().SetTimer(LifeTimeTimerHandle, this, &ThisClass::OnMagicCircleLifeTimeEnded, LifeTime, false);
}

void AMalogicMagicCircleInstance::OnMagicCircleLifeTimeEnded()
{
	if (!HasAuthority() || MagicCircleState == EMagicCircleState::Finished || MagicCircleState == EMagicCircleState::Destroyed)
	{
		return;
	}

	if (MagicCircleState == EMagicCircleState::Active && FinishAbilityClass)
	{
		bLifeTimeExpired = true;
		return;
	}

	if (MagicCircleState == EMagicCircleState::Ready || MagicCircleState == EMagicCircleState::Active)
	{
		FinishMagicCircle();
	}
}

void AMalogicMagicCircleInstance::OnAbilityFinished(const FAbilityEndedData& AbilityEndedData)
{
	if (!HasAuthority() || !FinishAbilityClass || !AbilityEndedData.AbilityThatEnded ||
		AbilityEndedData.AbilityThatEnded->GetClass() != FinishAbilityClass || MagicCircleState != EMagicCircleState::Active)
	{
		return;
	}

	//这个分支存疑
	if (AbilityEndedData.bWasCancelled)
	{
		UE_LOG(LogMalogic, Warning, TEXT("Finish ability [%s] was cancelled for magic circle [%s]. Destroying the instance."), *GetNameSafe(AbilityEndedData.AbilityThatEnded), *GetNameSafe(this));
		HandleOutOfHealth();
		return;
	}

	FinishMagicCircle();
}

void AMalogicMagicCircleInstance::FinishMagicCircle()
{
	if (!HasAuthority() || (MagicCircleState != EMagicCircleState::Ready && MagicCircleState != EMagicCircleState::Active))
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(LifeTimeTimerHandle);

	SetMagicCircleState(EMagicCircleState::Finished);
}

void AMalogicMagicCircleInstance::HandleOutOfHealth()
{
	if (!HasAuthority() || MagicCircleState == EMagicCircleState::Destroyed)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(BuildingTimerHandle);
	GetWorldTimerManager().ClearTimer(LifeTimeTimerHandle);

	SetMagicCircleState(EMagicCircleState::Destroyed);
}

void AMalogicMagicCircleInstance::HandleOutOfHealthEvent(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	HandleOutOfHealth();
}

void AMalogicMagicCircleInstance::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(BuildingTimerHandle);
	GetWorldTimerManager().ClearTimer(LifeTimeTimerHandle);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->OnAbilityEnded.RemoveAll(this);
		if (HasAuthority() && bAbilitySetGranted)
		{
			GrantedHandles.TakeFromAbilitySystem(AbilitySystemComponent);
			bAbilitySetGranted = false;
		}
	}

	if (HealthSet)
	{
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}
	if (HealthComponent)
	{
		HealthComponent->UninitializeFromAbilitySystem();
	}

	HealthSet = nullptr;
	CombatSet = nullptr;
	DeploymentInstigator = nullptr;

	Super::EndPlay(EndPlayReason);
}
