#include "Magic/DeployAbilities/MalogicGA_RotateToRaycastTarget.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Magic/MalogicMagicCircleDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGA_RotateToRaycastTarget)

bool UMalogicGA_RotateToRaycastTarget::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UMalogicMagicCircleDefinition* Definition = Cast<UMalogicMagicCircleDefinition>(GetSourceObject(Handle, ActorInfo));
	return Definition && !Definition->bIsPreDeploy;
}

bool UMalogicGA_RotateToRaycastTarget::CalculateDeployTransform(const FGameplayAbilityActorInfo* ActorInfo, FTransform& OutDeployTransform) const
{
	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	const APlayerController* PlayerController = AvatarPawn ? Cast<APlayerController>(AvatarPawn->GetController()) : nullptr;
	UWorld* World = AvatarPawn ? AvatarPawn->GetWorld() : nullptr;
	if (!AvatarPawn || !PlayerController || !World)
	{
		return false;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);

	constexpr float TraceDistance = 10000.0f;
	constexpr float DefaultDeployDistance = 100.0f;
	const FVector CameraDirection = CameraRotation.Vector().GetSafeNormal();
	if (CameraDirection.IsNearlyZero())
	{
		return false;
	}

	const FVector TraceEnd = CameraLocation + CameraDirection * TraceDistance;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RotateToRaycastTarget), false, AvatarPawn);
	const FVector TargetPoint = World->LineTraceSingleByChannel(
		HitResult,CameraLocation,
		TraceEnd,ECC_GameTraceChannel1,QueryParams)
		? HitResult.ImpactPoint
		: TraceEnd;

	const FVector Start = AvatarPawn->GetActorLocation()+ FVector(0,0,20.f);
	const FVector Direction = (TargetPoint - Start).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	const FVector DeployLocation = Start + Direction * DefaultDeployDistance;
	OutDeployTransform = FTransform(Direction.Rotation(), DeployLocation);
	return true;
}

