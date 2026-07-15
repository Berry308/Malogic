// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MalogicPawnExtensionComponent.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Character/MalogicPawnData.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "MalogicGameplayTags.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "Player/MalogicPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicPawnExtensionComponent)

UMalogicPawnExtensionComponent::UMalogicPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);

	PawnData = nullptr;
	AbilitySystemComponent = nullptr;
}

UMalogicPawnExtensionComponent* UMalogicPawnExtensionComponent::FindPawnExtensionComponent(const AActor* Actor)
{
	return Actor ? Actor->FindComponentByClass<UMalogicPawnExtensionComponent>() : nullptr;
}

void UMalogicPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf(Pawn, TEXT("MalogicPawnExtensionComponent can only be added to Pawn actors; owner is [%s]."), *GetNameSafe(GetOwner()));

	if (Pawn)
	{
		TArray<UActorComponent*> PawnExtensionComponents;
		Pawn->GetComponents(UMalogicPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
		ensureAlwaysMsgf(PawnExtensionComponents.Num() == 1, TEXT("Only one MalogicPawnExtensionComponent should exist on [%s]."), *GetNameSafe(Pawn));
	}
}

void UMalogicPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();

	TryInitializePawnData();
	TryInitializeAbilitySystem();
}

void UMalogicPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();

	Super::EndPlay(EndPlayReason);
}

void UMalogicPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, PawnData);
}

void UMalogicPawnExtensionComponent::InitializeAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent, AActor* InOwnerActor)
{
	check(InAbilitySystemComponent);
	check(InOwnerActor);

	if (AbilitySystemComponent == InAbilitySystemComponent && InAbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		return;
	}

	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}

	APawn* Pawn = GetPawnChecked<APawn>();
	AActor* ExistingAvatar = InAbilitySystemComponent->GetAvatarActor();
	if (ExistingAvatar && ExistingAvatar != Pawn)
	{
		ensureMsgf(!ExistingAvatar->HasAuthority(), TEXT("ASC [%s] is already using authoritative avatar [%s]."), *GetNameSafe(InAbilitySystemComponent), *GetNameSafe(ExistingAvatar));

		if (UMalogicPawnExtensionComponent* OtherExtension = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtension->UninitializeAbilitySystem();
		}
	}

	AbilitySystemComponent = InAbilitySystemComponent;
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);
	ApplyPawnDataToAbilitySystem();

	OnAbilitySystemInitialized.Broadcast();
}

void UMalogicPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		FGameplayTagContainer AbilityTypesToIgnore;
		AbilityTypesToIgnore.AddTag(MalogicGameplayTags::Ability_Behavior_SurvivesDeath);

		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor())
		{
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			AbilitySystemComponent->ClearActorInfo();
		}

		AbilitySystemComponent->SetTagRelationshipMapping(nullptr);

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void UMalogicPawnExtensionComponent::TryInitializeAbilitySystem()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		return;
	}

	if (AMalogicPlayerState* PlayerState = Pawn->GetPlayerState<AMalogicPlayerState>())
	{
		if (UMalogicAbilitySystemComponent* PlayerStateASC = PlayerState->GetMalogicAbilitySystemComponent())
		{
			InitializeAbilitySystem(PlayerStateASC, PlayerState);
		}
	}
}

void UMalogicPawnExtensionComponent::SetPawnData(const UMalogicPawnData* InPawnData)
{
	check(InPawnData);

	APawn* Pawn = GetPawnChecked<APawn>();
	if (!Pawn->HasAuthority())
	{
		return;
	}

	if (PawnData)
	{
		if (PawnData != InPawnData)
		{
			UE_LOG(LogMalogic, Error, TEXT("Trying to replace PawnData [%s] on pawn [%s] that already has PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		}
		return;
	}

	PawnData = InPawnData;
	ApplyPawnDataToAbilitySystem();
	Pawn->ForceNetUpdate();
	OnPawnDataInitialized.Broadcast();
}

void UMalogicPawnExtensionComponent::OnRep_PawnData()
{
	ApplyPawnDataToAbilitySystem();
	OnPawnDataInitialized.Broadcast();
}

void UMalogicPawnExtensionComponent::TryInitializePawnData()
{
	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->HasAuthority() || PawnData)
	{
		return;
	}

	if (const AMalogicPlayerState* PlayerState = Pawn->GetPlayerState<AMalogicPlayerState>())
	{
		if (const UMalogicPawnData* PlayerStatePawnData = PlayerState->GetPawnData<UMalogicPawnData>())
		{
			SetPawnData(PlayerStatePawnData);
		}
	}
}

void UMalogicPawnExtensionComponent::ApplyPawnDataToAbilitySystem()
{
	if (AbilitySystemComponent && PawnData)
	{
		AbilitySystemComponent->SetTagRelationshipMapping(PawnData->TagRelationshipMapping);
	}
}

void UMalogicPawnExtensionComponent::HandleControllerChanged()
{
	if (AbilitySystemComponent && AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		AbilitySystemComponent->RefreshAbilityActorInfo();
	}

	if (const APawn* Pawn = GetPawn<APawn>(); Pawn && Pawn->GetController())
	{
		TryInitializePawnData();
		TryInitializeAbilitySystem();
	}
}

void UMalogicPawnExtensionComponent::HandlePlayerStateReplicated()
{
	TryInitializePawnData();
	TryInitializeAbilitySystem();
}

void UMalogicPawnExtensionComponent::SetupPlayerInputComponent()
{
	OnPawnInputComponentReady.Broadcast();
	TryInitializePawnData();
	TryInitializeAbilitySystem();
}

void UMalogicPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void UMalogicPawnExtensionComponent::OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}

void UMalogicPawnExtensionComponent::OnPawnDataInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnPawnDataInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnPawnDataInitialized.Add(Delegate);
	}

	if (PawnData)
	{
		Delegate.Execute();
	}
}

void UMalogicPawnExtensionComponent::OnPawnInputComponentReady_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate)
{
	if (!OnPawnInputComponentReady.IsBoundToObject(Delegate.GetUObject()))
	{
		OnPawnInputComponentReady.Add(Delegate);
	}

	if (const APawn* Pawn = GetPawn<APawn>(); Pawn && Pawn->InputComponent)
	{
		Delegate.Execute();
	}
}


