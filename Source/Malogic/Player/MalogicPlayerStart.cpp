// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/MalogicPlayerStart.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicPlayerStart)

AMalogicPlayerStart::AMalogicPlayerStart(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

EMalogicPlayerStartLocationOccupancy AMalogicPlayerStart::GetLocationOccupancy(AController* Controller) const
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World)
	{
		return EMalogicPlayerStartLocationOccupancy::Full;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	const TSubclassOf<APawn> PawnClass = GameMode ? GameMode->GetDefaultPawnClassForController(Controller) : nullptr;
	const APawn* PawnToFit = PawnClass ? GetDefault<APawn>(PawnClass) : nullptr;
	if (!PawnToFit)
	{
		return EMalogicPlayerStartLocationOccupancy::Full;
	}

	FVector SpawnLocation = GetActorLocation();
	const FRotator SpawnRotation = GetActorRotation();
	if (!World->EncroachingBlockingGeometry(PawnToFit, SpawnLocation, SpawnRotation, nullptr))
	{
		return EMalogicPlayerStartLocationOccupancy::Empty;
	}

	return World->FindTeleportSpot(PawnToFit, SpawnLocation, SpawnRotation)
		? EMalogicPlayerStartLocationOccupancy::Partial
		: EMalogicPlayerStartLocationOccupancy::Full;
}

bool AMalogicPlayerStart::IsClaimed() const
{
	return ClaimingController != nullptr;
}

bool AMalogicPlayerStart::TryClaim(AController* OccupyingController)
{
	if (!OccupyingController || IsClaimed())
	{
		return false;
	}

	ClaimingController = OccupyingController;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ExpirationTimerHandle, FTimerDelegate::CreateUObject(this, &ThisClass::CheckUnclaimed), ExpirationCheckInterval, true);
	}

	return true;
}

void AMalogicPlayerStart::CheckUnclaimed()
{
	if (ClaimingController && ClaimingController->GetPawn() && GetLocationOccupancy(ClaimingController) == EMalogicPlayerStartLocationOccupancy::Empty)
	{
		ClaimingController = nullptr;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
		}
	}
}
