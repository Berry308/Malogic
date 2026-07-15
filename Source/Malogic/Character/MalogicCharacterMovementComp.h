// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "NativeGameplayTags.h"

#include "MalogicCharacterMovementComp.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MovementStopped);

USTRUCT(BlueprintType)
struct FMalogicCharacterGroundInfo
{
	GENERATED_BODY()

	FMalogicCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{
	}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};

/** Character movement implementation with GAS movement blocking and cached ground data. */
UCLASS(Config = Game)
class MALOGIC_API UMalogicCharacterMovementComp : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UMalogicCharacterMovementComp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void SimulateMovement(float DeltaTime) override;
	virtual bool CanAttemptJump() const override;

	UFUNCTION(BlueprintCallable, Category = "Malogic|CharacterMovement")
	const FMalogicCharacterGroundInfo& GetGroundInfo();

	void SetReplicatedAcceleration(const FVector& InAcceleration);

	virtual FRotator GetDeltaRotation(float DeltaTime) const override;
	virtual float GetMaxSpeed() const override;

protected:
	virtual void InitializeComponent() override;

private:
	FMalogicCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
