// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemComponent.h"
#include "MalogicAttributeSet.h"
#include "NativeGameplayTags.h"

#include "MalogicHealthSet.generated.h"

class UObject;
struct FFrame;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Damage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageImmunity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageSelfDestruct);//自毁伤害
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_FellOutOfWorld);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Lyra_Damage_Message);

struct FGameplayEffectModCallbackData;

/**
 *  HealthSet
 *
 *	Class that defines attributes that are necessary for taking damage.
 *	Attribute examples include: health, shields, and resistances.
 *  在PlayerState中被创建
 */
UCLASS()
class MALOGIC_API UMalogicHealthSet : public UMalogicAttributeSet
{
	GENERATED_BODY()
	
public:
	UMalogicHealthSet();

	//Getter获取的是CurrentValue，Setter设置的是BaseValue
	ATTRIBUTE_ACCESSORS(UMalogicHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UMalogicHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UMalogicHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UMalogicHealthSet, Damage);

	// Delegate when health changes due to damage/healing, some information may be missing on the client
	mutable FMRAttributeEvent OnHealthChanged;

	// Delegate when max health changes
	mutable FMRAttributeEvent OnMaxHealthChanged;

	// Delegate to broadcast when the health attribute reaches zero
	mutable FMRAttributeEvent OnOutOfHealth;

protected:
	//如果你在 OnRep 函数中声明了一个与同步属性类型相同的参数，引擎会自动将同步发生前的本地值（Old Value）作为参数传入。
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	//在GE执行前，对应用目标进行检查（如是否包含伤害免疫标签，返回是否可以执行的bool
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	/*
	* 实际对AttributeData作出更改的函数
	* GAS在每次GameplayEffect完整应用（包括所有 Modifier 计算、Execution 执行等）后自动调用它。
	*/
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	//下列函数均在PostGameplayEffectExecute中进行属性设置后调用，详情见源码
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:

	// The current health attribute.  The health will be capped by the max health attribute.  Health is hidden from modifiers so only executions can modify it.
	// HideFromModifiers隐藏该属性在编辑器（GameplayEffect 编辑器里的 Modifier 下拉列表）中的显示。 只有GameplayEffectExecutionCalculation可以修改该属性
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "MazeRunner|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	// The current max health attribute.  Max health is an attribute since gameplay effects can modify it.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "MazeRunner|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	// Used to track when the health reaches 0.
	bool bOutOfHealth;

	// Store the health before any changes 
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	Meta Attribute (please keep attributes that aren't 'stateful' below 
	// -------------------------------------------------------------------

	// Incoming healing. This is mapped directly to +Health
	UPROPERTY(BlueprintReadOnly, Category = "Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Healing;

	// Incoming damage. This is mapped directly to -Health
	// HideFromModifiers隐藏该属性在编辑器（GameplayEffect 编辑器里的 Modifier 下拉列表）中的显示
	UPROPERTY(BlueprintReadOnly, Category = "Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Damage;
};
