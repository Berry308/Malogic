// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AbilityCost.h"
#include "ScalableFloat.h"
#include "AbilityCost_Magic.generated.h"

/** Ability cost that consumes the owner's current magic value. */
UCLASS()
class MALOGIC_API UAbilityCost_Magic : public UAbilityCost
{
	GENERATED_BODY()

public:
	virtual bool CheckCost(const UMalogicGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UMalogicGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
	/** The amount of magic to consume, evaluated at the ability's level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs, meta = (ClampMin = "0.0"))
	FScalableFloat MagicCost = FScalableFloat(0.0f);
};
