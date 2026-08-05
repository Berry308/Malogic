#include "AbilitySystem/Executions/MalogicDamageExecution.h"

#include "AbilitySystem/Attributes/MalogicCombatSet.h"
#include "AbilitySystem/Attributes/MalogicHealthSet.h"
#include "AbilitySystem/MalogicAbilitySourceInterface.h"
#include "AbilitySystem/MalogicGameplayEffectContext.h"
#include "AbilitySystemComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicDamageExecution)

namespace
{
	struct FDamageStatics
	{
		FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

		FDamageStatics()
			: BaseDamageDef(UMalogicCombatSet::GetBaseDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true)
		{
		}
	};

	const FDamageStatics& DamageStatics()
	{
		static const FDamageStatics Statics;
		return Statics;
	}
}

UMalogicDamageExecution::UMalogicDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
}

void UMalogicDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);

	float PhysicalMaterialAttenuation = 1.0f;
	float DistanceAttenuation = 1.0f;
	if (const FMalogicGameplayEffectContext* EffectContext = FMalogicGameplayEffectContext::ExtractEffectContext(Spec.GetContext()))
	{
		const AActor* EffectCauser = EffectContext->GetEffectCauser();
		const FHitResult* HitResult = EffectContext->GetHitResult();
		AActor* HitActor = HitResult ? HitResult->HitObjectHandle.FetchActor() : nullptr;

		FVector ImpactLocation = FVector::ZeroVector;
		if (HitActor)
		{
			ImpactLocation = HitResult->ImpactPoint;
		}
		else if (UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent())
		{
			HitActor = TargetAbilitySystemComponent->GetAvatarActor_Direct();
			if (HitActor)
			{
				ImpactLocation = HitActor->GetActorLocation();
			}
		}

		double Distance = WORLD_MAX;
		if (EffectContext->HasOrigin())
		{
			Distance = FVector::Dist(EffectContext->GetOrigin(), ImpactLocation);
		}
		else if (EffectCauser)
		{
			Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
		}
		else
		{
			UE_LOG(LogMalogic, Warning, TEXT("Damage calculation cannot determine a source location for [%s]; using WORLD_MAX distance."), *GetPathNameSafe(Spec.Def));
		}

		if (const IMalogicAbilitySourceInterface* AbilitySource = EffectContext->GetAbilitySource())
		{
			if (const UPhysicalMaterial* PhysicalMaterial = EffectContext->GetPhysicalMaterial())
			{
				PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(PhysicalMaterial, SourceTags, TargetTags);
			}

			DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
		}
	}

	DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);
	const float DamageDone = FMath::Max(BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation, 0.0f);
	if (DamageDone > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UMalogicHealthSet::GetDamageAttribute(), EGameplayModOp::Additive, DamageDone));
	}
#endif
}
