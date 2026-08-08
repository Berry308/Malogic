// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "MalogicAttributeSet.h"

#include "MalogicCombatSet.generated.h"

/**
 * 提供基础战斗属性集，如伤害量和治疗量，定义在PlayerState中，在DamageExecution中被捕获使用
 */
UCLASS(BlueprintType)
class MALOGIC_API UMalogicCombatSet : public UMalogicAttributeSet
{
	GENERATED_BODY()
public:

	UMalogicCombatSet();

	ATTRIBUTE_ACCESSORS(UMalogicCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UMalogicCombatSet, BaseHeal);

protected:

	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

private:

	// The base amount of damage to apply in the damage execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "MazeRunner|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	// The base amount of healing to apply in the heal execution.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "MazeRunner|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;
};
