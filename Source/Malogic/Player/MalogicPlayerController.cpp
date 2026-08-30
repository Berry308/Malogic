// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/MalogicAbilitySystemComponent.h"
//#include "Blueprint/UserWidget.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Player/MalogicPlayerState.h"
#include "Weapon/MagicWeaponStateComponent.h"

AMalogicPlayerController::AMalogicPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void AMalogicPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AMalogicPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	SetActorHiddenInGame(false);
}

void AMalogicPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AMalogicPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
}

AMalogicPlayerState* AMalogicPlayerController::GetMalogicPlayerState() const
{
	return CastChecked<AMalogicPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UMalogicAbilitySystemComponent* AMalogicPlayerController::GetMalogicAbilitySystemComponent() const
{
	return Cast<UMalogicAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState));
}

//void AMalogicPlayerController::SetUIInputMode(UUserWidget* WidgetToFocus)
//{
//	if (!WidgetToFocus)
//	{
//		return;
//	}
//
//	FInputModeUIOnly InputMode;
//	InputMode.SetWidgetToFocus(WidgetToFocus->TakeWidget());
//	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
//
//	SetInputMode(InputMode);
//	bShowMouseCursor = true;
//}
//
//void AMalogicPlayerController::SetGameInputMode()
//{
//	FInputModeGameOnly InputMode;
//	SetInputMode(InputMode);
//	bShowMouseCursor = false;
//}

void AMalogicPlayerController::OnPlayerStateChanged()
{
	// Empty, place for derived classes to implement without having to hook all the other events.
}

void AMalogicPlayerController::BroadcastOnPlayerStateChanged()
{
	OnPlayerStateChanged();

	LastSeenPlayerState = PlayerState;
	OnMalogicPlayerStateChanged.Broadcast();
}

void AMalogicPlayerController::InitPlayerState()
{
	Super::InitPlayerState();

	BroadcastOnPlayerStateChanged();
}

void AMalogicPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();

	BroadcastOnPlayerStateChanged();
}

void AMalogicPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	BroadcastOnPlayerStateChanged();

	if (GetWorld()->IsNetMode(NM_Client))
	{
		if (UMalogicAbilitySystemComponent* MalogicASC = GetMalogicAbilitySystemComponent())
		{
			MalogicASC->RefreshAbilityActorInfo();
			MalogicASC->TryActivateAbilitiesOnSpawn();
		}
	}
}

void AMalogicPlayerController::SetPlayer(UPlayer* InPlayer)
{
	Super::SetPlayer(InPlayer);
}

void AMalogicPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void AMalogicPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (UMalogicAbilitySystemComponent* MalogicASC = GetMalogicAbilitySystemComponent())
	{
		MalogicASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AMalogicPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AMalogicPlayerController::OnUnPossess()
{
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UMalogicPawnExtensionComponent* PawnExtensionComponent = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(PawnBeingUnpossessed))
		{
			PawnExtensionComponent->UninitializeAbilitySystem();
		}
	}

	Super::OnUnPossess();
}

