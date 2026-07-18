// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicHeroComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MalogicLogChannels.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Character/MalogicCharacter.h"
#include "Character/MalogicPawnData.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "Player/MalogicPlayerController.h"
#include "Player/MalogicPlayerState.h"
#include "Player/MalogicLocalPlayer.h"
#include "Input/MalogicInputConfig.h"
#include "Input/MalogicInputComponent.h"
#include "MalogicGameplayTags.h"


namespace MalogicHero
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

const FName UMalogicHeroComponent::NAME_BindInputsNow("BindInputsNow");
const FName UMalogicHeroComponent::NAME_ActorFeatureName("Hero");

UMalogicHeroComponent::UMalogicHeroComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/*AbilityCameraMode = nullptr;*/
	bReadyToBindInputs = false;
}

void UMalogicHeroComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogMalogic, Error, TEXT("[UMalogicHeroComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));
	}
	else
	{
		if (UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(GetOwner()))
		{
			const FSimpleMulticastDelegate::FDelegate InitializeHeroDelegate = FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::TryInitializeHero);
			PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(InitializeHeroDelegate);
			PawnExtension->OnPawnDataInitialized_RegisterAndCall(InitializeHeroDelegate);
			PawnExtension->OnPawnInputComponentReady_RegisterAndCall(InitializeHeroDelegate);
		}
	}
}

void UMalogicHeroComponent::BeginPlay()
{
	Super::BeginPlay();

	TryInitializeHero();
}

void UMalogicHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Super::EndPlay(EndPlayReason);
}

void UMalogicHeroComponent::TryInitializeHero()
{
	if (bReadyToBindInputs)
	{
		return;
	}

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled() || !Pawn->InputComponent)
	{
		return;
	}

	const APlayerController* PlayerController = GetController<APlayerController>();
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	const UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtension || !PawnExtension->GetPawnData() || !PawnExtension->GetMalogicAbilitySystemComponent())
	{
		return;
	}

	InitializePlayerInput(Pawn->InputComponent);
}

//添加默认输入映射上下文
void UMalogicHeroComponent::InitializePlayerInput(UInputComponent* PlayerInputComponent)
{
	//UE_LOG(LogMalogic, Warning, TEXT("%s HeroComponent is InitializePlayerInput"), *GetOwningActor()->GetName());
	check(PlayerInputComponent);

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	const UMalogicPawnData* PawnData = PawnExtension ? PawnExtension->GetPawnData() : nullptr;
	if (!PawnData)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UMalogicLocalPlayer* LP = Cast<UMalogicLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	const UMalogicInputConfig* InputConfig = PawnData->InputConfig ? PawnData->InputConfig.Get() : DefaultInputConfig;
	if (!InputConfig)
	{
		UE_LOG(LogMalogic, Error, TEXT("PawnData [%s] has no InputConfig and HeroComponent [%s] has no default input config."), *GetNameSafe(PawnData), *GetNameSafe(this));
		return;
	}

	Subsystem->ClearAllMappings();


	for (const FInputMappingContextAndPriority& Mapping : DefaultInputMappings)
	{
		if (UInputMappingContext* IMC = Mapping.InputMapping.LoadSynchronous())
		{
			if (Mapping.bRegisterWithSettings)
			{
				if (UEnhancedInputUserSettings* Settings = Subsystem->GetUserSettings())
				{
					Settings->RegisterInputMappingContext(IMC);
				}

				FModifyContextOptions Options = {};
				Options.bIgnoreAllPressedKeysUntilRelease = false;
				// Actually add the config to the local player							
				Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
			}
		}
	}

	// The Lyra Input Component has some additional functions to map Gameplay Tags to an Input Action.
	// If you want this functionality but still want to change your input component class, make it a subclass
	// of the UMalogicInputComponent or modify this component accordingly.
	UMalogicInputComponent* InputComp = Cast<UMalogicInputComponent>(PlayerInputComponent);
	if (!ensureMsgf(InputComp, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UMalogicInputComponent or a subclass of it.")))
	{
		return;
	}

	// Add the key mappings that may have been set by the player
	InputComp->AddInputMappings(InputConfig, Subsystem);

	// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
	// be triggered directly by these input actions Triggered events.
	TArray<uint32> BindHandles;
	InputComp->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

	InputComp->BindNativeAction(InputConfig, MalogicGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
	InputComp->BindNativeAction(InputConfig, MalogicGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
	InputComp->BindNativeAction(InputConfig, MalogicGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
	InputComp->BindNativeAction(InputConfig, MalogicGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);


	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
		UE_LOG(LogMalogic, Log, TEXT("HeroComponent is Ready To Bind Inputs"));
	}
	UE_LOG(LogMalogic, Warning, TEXT("HeroComponent is InitializePlayerInput"));
}

//被GameFeatureAction_InputBinding所使用
void UMalogicHeroComponent::AddAdditionalInputConfig(const UMalogicInputConfig* InputConfig)
{
	TArray<uint32> BindHandles;

	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

	UMalogicInputComponent* InputComp = Pawn->FindComponentByClass<UMalogicInputComponent>();
	if (ensureMsgf(InputComp, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UMalogicInputComponent or a subclass of it.")))
	{
		InputComp->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);
	}
}

void UMalogicHeroComponent::RemoveAdditionalInputConfig(const UMalogicInputConfig* InputConfig)
{
	//@TODO: Implement me!
}

bool UMalogicHeroComponent::IsReadyToBindInputs() const
{
	return bReadyToBindInputs;
}

void UMalogicHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
{
	if (const APawn* Pawn = GetPawn<APawn>())
	{
		if (const UMalogicPawnExtensionComponent* PawnExtComp = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			if (UMalogicAbilitySystemComponent* MalogicASC = PawnExtComp->GetMalogicAbilitySystemComponent())
			{
				MalogicASC->AbilityInputTagPressed(InputTag);
			}
		}
	}
}

void UMalogicHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (const UMalogicPawnExtensionComponent* PawnExtComp = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
	{
		if (UMalogicAbilitySystemComponent* MalogicASC = PawnExtComp->GetMalogicAbilitySystemComponent())
		{
			MalogicASC->AbilityInputTagReleased(InputTag);
		}
	}
}

void UMalogicHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running

	if (Controller)
	{
		const FVector2D Value = InputActionValue.Get<FVector2D>();
		const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

		if (Value.X != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);
			Pawn->AddMovementInput(MovementDirection, Value.X);
		}

		if (Value.Y != 0.0f)
		{
			const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
			Pawn->AddMovementInput(MovementDirection, Value.Y);
		}
	}
}

void UMalogicHeroComponent::Input_LookMouse(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y);
	}
}

void UMalogicHeroComponent::Input_LookStick(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();

	if (!Pawn)
	{
		return;
	}

	const FVector2D Value = InputActionValue.Get<FVector2D>();

	const UWorld* World = GetWorld();
	check(World);

	if (Value.X != 0.0f)
	{
		Pawn->AddControllerYawInput(Value.X * MalogicHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * MalogicHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UMalogicHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (AMalogicCharacter* Character = GetPawn<AMalogicCharacter>())
	{
		//Character->ToggleCrouch();
	}
}
