// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicHeroComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MalogicLogChannels.h"
#include "PlayerMappableInputConfig.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "InputMappingContext.h"

//#include "AbilitySystem/MRAbilitySystemComponent.h"
#include "Character/MalogicCharacter.h"
#include "Player/MalogicPlayerController.h"
#include "Player/MalogicPlayerState.h"
#include "Player/MalogicLocalPlayer.h"
#include "Input/MalogicInputConfig.h"
#include "Input/MalogicInputComponent.h"
#include "MalogicGameplayTags.h"


namespace MRHero
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
		// Register with the init state system early, this will only work if this is a game world
		//RegisterInitStateFeature();
	}
}

void UMalogicHeroComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializePlayerInput(GetPawn<APawn>()->InputComponent);
}

void UMalogicHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Super::EndPlay(EndPlayReason);
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

	const APlayerController* PC = GetController<APlayerController>();
	check(PC);

	const UMalogicLocalPlayer* LP = Cast<UMalogicLocalPlayer>(PC->GetLocalPlayer());
	check(LP);

	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	check(Subsystem);

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
	// of the UMRInputComponent or modify this component accordingly.
	UMalogicInputComponent* InputComp = Cast<UMalogicInputComponent>(PlayerInputComponent);
	if (ensureMsgf(InputComp, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UMRInputComponent or a subclass of it.")))
	{
		// Add the key mappings that may have been set by the player
		InputComp->AddInputMappings(DefaultInputConfig, Subsystem);

		// This is where we actually bind and input action to a gameplay tag, which means that Gameplay Ability Blueprints will
		// be triggered directly by these input actions Triggered events. 
		TArray<uint32> BindHandles;
		InputComp->BindAbilityActions(DefaultInputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

		InputComp->BindNativeAction(DefaultInputConfig, MalogicGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move, /*bLogIfNotFound=*/ false);
		InputComp->BindNativeAction(DefaultInputConfig, MalogicGameplayTags::InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse, /*bLogIfNotFound=*/ false);
		InputComp->BindNativeAction(DefaultInputConfig, MalogicGameplayTags::InputTag_Look_Stick, ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick, /*bLogIfNotFound=*/ false);
		InputComp->BindNativeAction(DefaultInputConfig, MalogicGameplayTags::InputTag_Crouch, ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch, /*bLogIfNotFound=*/ false);
		//InputComp->BindNativeAction(DefaultInputConfig, MalogicGameplayTags::InputTag_AutoRun, ETriggerEvent::Triggered, this, &ThisClass::Input_AutoRun, /*bLogIfNotFound=*/ false);
	}


	if (ensure(!bReadyToBindInputs))
	{
		bReadyToBindInputs = true;
		UE_LOG(LogMalogic, Log, TEXT("HeroComponent is Ready To Bind Inputs"));
	}
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
	if (ensureMsgf(InputComp, TEXT("Unexpected Input Component class! The Gameplay Abilities will not be bound to their inputs. Change the input component to UMRInputComponent or a subclass of it.")))
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

//void UMalogicHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag)
//{
//	if (const APawn* Pawn = GetPawn<APawn>())
//	{
//		if (UMRAbilitySystemComponent* MRASC = PawnExtComp->GetMRAbilitySystemComponent())
//		{
//			MRASC->AbilityInputTagPressed(InputTag);
//		}
//	}
//}

//void UMalogicHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag)
//{
//	const APawn* Pawn = GetPawn<APawn>();
//	if (!Pawn)
//	{
//		return;
//	}
//
//	if (const UMRPawnExtensionComponent* PawnExtComp = UMRPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
//	{
//		if (UMRAbilitySystemComponent* MRASC = PawnExtComp->GetMRAbilitySystemComponent())
//		{
//			MRASC->AbilityInputTagReleased(InputTag);
//		}
//	}
//}

void UMalogicHeroComponent::Input_Move(const FInputActionValue& InputActionValue)
{
	APawn* Pawn = GetPawn<APawn>();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// If the player has attempted to move again then cancel auto running
	/*if (AMRPlayerController* MRController = Cast<AMRPlayerController>(Controller))
	{
		MRController->SetIsAutoRunning(false);
	}*/

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
		Pawn->AddControllerYawInput(Value.X * MRHero::LookYawRate * World->GetDeltaSeconds());
	}

	if (Value.Y != 0.0f)
	{
		Pawn->AddControllerPitchInput(Value.Y * MRHero::LookPitchRate * World->GetDeltaSeconds());
	}
}

void UMalogicHeroComponent::Input_Crouch(const FInputActionValue& InputActionValue)
{
	if (AMalogicCharacter* Character = GetPawn<AMalogicCharacter>())
	{
		//Character->ToggleCrouch();
	}
}
