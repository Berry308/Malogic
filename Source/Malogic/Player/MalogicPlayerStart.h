// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

#include "MalogicPlayerStart.generated.h"

class AController;

UENUM(BlueprintType)
enum class EMalogicPlayerStartLocationOccupancy : uint8
{
	Empty,
	Partial,
	Full
};

/** Player start with server-side occupancy checks for the current controller's pawn class. */
UCLASS(Config = Game)
class MALOGIC_API AMalogicPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	AMalogicPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	const FGameplayTagContainer& GetGameplayTags() const { return StartPointTags; }
	EMalogicPlayerStartLocationOccupancy GetLocationOccupancy(AController* Controller) const;
	bool IsClaimed() const;
	bool TryClaim(AController* OccupyingController);

protected:
	void CheckUnclaimed();

	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController;

	UPROPERTY(EditDefaultsOnly, Category = "Malogic|Player Start Claiming")
	float ExpirationCheckInterval = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Malogic|Player Start")
	FGameplayTagContainer StartPointTags;

	FTimerHandle ExpirationTimerHandle;
};
