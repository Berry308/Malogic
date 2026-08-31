#include "AbilitySystem/Abilities/Cost/AbilityCost_Magic.h"

#include "AbilitySystem/Attributes/MalogicMagicSet.h"
#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "MalogicGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityCost_Magic)

bool UAbilityCost_Magic::CheckCost(const UMalogicGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Ability || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	if (!AbilitySystemComponent->GetSet<UMalogicMagicSet>())
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(MalogicGameplayTags::Ability_ActivateFail_Cost);
		}

		return false;
	}

	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_Gameplay_MagicUnlimited))
	{
		return true;
	}

	const float Cost = FMath::Max(0.0f, MagicCost.GetValueAtLevel(Ability->GetAbilityLevel(Handle, ActorInfo)));
	if (AbilitySystemComponent->GetNumericAttribute(UMalogicMagicSet::GetMagicValueAttribute()) < Cost)
	{
		if (OptionalRelevantTags)
		{
			OptionalRelevantTags->AddTag(MalogicGameplayTags::Ability_ActivateFail_Cost);
		}

		return false;
	}

	return true;
}

void UAbilityCost_Magic::ApplyCost(const UMalogicGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!Ability || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	if (!AbilitySystemComponent->GetSet<UMalogicMagicSet>())
	{
		return;
	}

	const float Cost = FMath::Max(0.0f, MagicCost.GetValueAtLevel(Ability->GetAbilityLevel(Handle, ActorInfo)));
	if (Cost <= 0.0f)
	{
		return;
	}

	// Use an instant effect so MagicSet processes the meta attribute and broadcasts its normal change events.
	UGameplayEffect* MagicCostEffect = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	MagicCostEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UMalogicMagicSet::GetMagicConsumeAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Cost));
	MagicCostEffect->Modifiers.Add(Modifier);

	const FGameplayEffectSpec CostSpec(MagicCostEffect, Ability->GetContextFromOwner(FGameplayAbilityTargetDataHandle()), Ability->GetAbilityLevel(Handle, ActorInfo));
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(CostSpec);
}

