// Fill out your copyright notice in the Description page of Project Settings.


#include "Magic/MagicAbilities/MalogicGA_LightBeamShoot.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Magic/MalogicMagicCircleDefinition.h"
#include "Magic/MalogicMagicCircleInstance.h"
#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGA_LightBeamShoot)

namespace
{
	constexpr float MagicCircleBeamOriginOffset = 10.0f;
	constexpr float LightBeamDebugDrawDuration = 2.0f;
	const FName LightBeamTraceStatName(TEXT("MagicLightBeam"));
}

void UMalogicGA_LightBeamShoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AMalogicMagicCircleInstance* MagicCircleInstance = Cast<AMalogicMagicCircleInstance>(ActorInfo->AvatarActor.Get());
	const UMalogicMagicCircleDefinition* Definition = Cast<UMalogicMagicCircleDefinition>(GetSourceObject(Handle, ActorInfo));
	UMalogicAbilitySystemComponent* AbilitySystemComponent = Cast<UMalogicAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());
	UWorld* World = GetWorld();

	if (!MagicCircleInstance || !Definition || !AbilitySystemComponent || !World)
	{
		UE_LOG(LogMalogic, Error, TEXT("Light beam ability [%s] is missing its magic circle, definition, ability system component, or world."), *GetNameSafe(this));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const float MaxShootDistance = Definition->MaxShootDistance;
	const float BeamRadius = Definition->BeamRadius;

	const FVector Forward = MagicCircleInstance->GetActorForwardVector().GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		UE_LOG(LogMalogic, Error, TEXT("Light beam ability [%s] could not determine a valid forward direction for magic circle [%s]."), *GetNameSafe(this), *GetNameSafe(MagicCircleInstance));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector BeamStart = MagicCircleInstance->GetActorLocation() + Forward * MagicCircleBeamOriginOffset;
	const FVector MaximumBeamEnd = BeamStart + Forward * MaxShootDistance;

	FCollisionQueryParams QueryParams(LightBeamTraceStatName, true, MagicCircleInstance);
	QueryParams.AddIgnoredActor(MagicCircleInstance);

	if (AActor* DeploymentInstigator = MagicCircleInstance->GetDeploymentInstigator())
	{
		QueryParams.AddIgnoredActor(DeploymentInstigator);

		TArray<AActor*> AttachedActors;
		DeploymentInstigator->GetAttachedActors(AttachedActors);
		QueryParams.AddIgnoredActors(AttachedActors);
	}

	FHitResult HitResult;
	const bool bHit = World->SweepSingleByChannel(
		HitResult,BeamStart,
		MaximumBeamEnd,FQuat::Identity,
		ECC_GameTraceChannel1,FCollisionShape::MakeSphere(BeamRadius),
		QueryParams);

	const FVector BeamEnd = bHit ? HitResult.ImpactPoint : MaximumBeamEnd;
	const float BeamLength = FVector::Dist(BeamStart, BeamEnd);

#if ENABLE_DRAW_DEBUG
	//const FColor TraceColor = bHit ? FColor::Green : FColor::Red;
	//DrawDebugLine(World, BeamStart, MaximumBeamEnd, FColor::White, false, LightBeamDebugDrawDuration, 0, 0.5f);
	//DrawDebugLine(World, BeamStart, BeamEnd, TraceColor, false, LightBeamDebugDrawDuration, 0, 2.0f);
	//DrawDebugSphere(World, BeamStart, BeamRadius, 12, FColor::Yellow, false, LightBeamDebugDrawDuration, 0, 1.0f);
	//DrawDebugSphere(World, BeamEnd, BeamRadius, 12, TraceColor, false, LightBeamDebugDrawDuration, 0, 1.5f);

	//if (bHit)
	//{
	//	DrawDebugPoint(World, HitResult.ImpactPoint, 12.0f, FColor::White, false, LightBeamDebugDrawDuration);
	//	DrawDebugLine(World, HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * 30.0f, FColor::Cyan, false, LightBeamDebugDrawDuration, 0, 1.5f);
	//}
#endif

	if (bHit)
	{
		K2_OnMagicHit(HitResult);
	}

	const FGameplayTag GameplayCueTag = FGameplayTag::RequestGameplayTag(FName(TEXT("GameplayCue.Magic.LightBeam")), false);
	if (GameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		CueParameters.Location = BeamStart;
		CueParameters.Normal = Forward;
		CueParameters.RawMagnitude = BeamLength;
		CueParameters.Instigator = MagicCircleInstance;
		CueParameters.EffectCauser = MagicCircleInstance;
		CueParameters.SourceObject = Definition;
		CueParameters.EffectContext = AbilitySystemComponent->MakeEffectContext();

		CueParameters.EffectContext.AddHitResult(HitResult);

		AbilitySystemComponent->ExecuteGameplayCue(GameplayCueTag, CueParameters);
	}
	else
	{
		UE_LOG(LogMalogic, Warning, TEXT("Light beam ability [%s] could not find GameplayCue tag [GameplayCue.Magic.LightBeam]."), *GetNameSafe(this));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

