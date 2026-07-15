// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/MalogicCharacterMovementComp.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicCharacterMovementComp)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");

namespace MalogicCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVarGroundTraceDistance(TEXT("MalogicCharacter.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
}

UMalogicCharacterMovementComp::UMalogicCharacterMovementComp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMalogicCharacterMovementComp::SimulateMovement(float DeltaTime)
{
	if (bHasReplicatedAcceleration)
	{
		const FVector OriginalAcceleration = Acceleration;
		Super::SimulateMovement(DeltaTime);
		Acceleration = OriginalAcceleration;
		return;
	}

	Super::SimulateMovement(DeltaTime);
}

bool UMalogicCharacterMovementComp::CanAttemptJump() const
{
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling());
}

void UMalogicCharacterMovementComp::InitializeComponent()
{
	Super::InitializeComponent();
}

const FMalogicCharacterGroundInfo& UMalogicCharacterMovementComp::GetGroundInfo()
{
	if (!CharacterOwner || GFrameCounter == CachedGroundInfo.LastUpdateFrame)
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComponent = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComponent);

		const float CapsuleHalfHeight = CapsuleComponent->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn;
		const FVector TraceStart = GetActorLocation();
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, TraceStart.Z - MalogicCharacter::GroundTraceDistance - CapsuleHalfHeight);

		FCollisionQueryParams QueryParameters(SCENE_QUERY_STAT(MalogicCharacterMovementComp_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParameters;
		InitCollisionParams(QueryParameters, ResponseParameters);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParameters, ResponseParameters);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = MalogicCharacter::GroundTraceDistance;
		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max(HitResult.Distance - CapsuleHalfHeight, 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;
	return CachedGroundInfo;
}

void UMalogicCharacterMovementComp::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}

FRotator UMalogicCharacterMovementComp::GetDeltaRotation(float DeltaTime) const
{
	if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return FRotator::ZeroRotator;
		}
	}

	return Super::GetDeltaRotation(DeltaTime);
}

float UMalogicCharacterMovementComp::GetMaxSpeed() const
{
	if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
		{
			return 0.0f;
		}
	}

	return Super::GetMaxSpeed();
}
