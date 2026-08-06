#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MalogicAttributeSet.h"
#include "NativeGameplayTags.h"

#include "MalogicMagicSet.generated.h"

struct FGameplayEffectModCallbackData;
class FLifetimeProperty;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_MagicUnlimited);

/**
 * Attribute set that owns the character's current and maximum magic values.
 */
UCLASS()
class MALOGIC_API UMalogicMagicSet : public UMalogicAttributeSet
{
	GENERATED_BODY()

public:
	UMalogicMagicSet();

	ATTRIBUTE_ACCESSORS(UMalogicMagicSet, MagicValue);
	ATTRIBUTE_ACCESSORS(UMalogicMagicSet, MaxMagicValue);
	ATTRIBUTE_ACCESSORS(UMalogicMagicSet, MagicConsume);
	ATTRIBUTE_ACCESSORS(UMalogicMagicSet, MagicRecovery);

	/** Broadcast when the current magic value changes. */
	mutable FMRAttributeEvent OnMagicValueChanged;

	/** Broadcast when the maximum magic value changes. */
	mutable FMRAttributeEvent OnMaxMagicValueChanged;

	/** Broadcast when the current magic value reaches zero. */
	mutable FMRAttributeEvent OnOutOfMagic;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UFUNCTION()
	void OnRep_MagicValue(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMagicValue(const FGameplayAttributeData& OldValue);

	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MagicValue, Category = "MazeRunner|Magic", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData MagicValue;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMagicValue, Category = "MazeRunner|Magic", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxMagicValue;

	bool bOutOfMagic;
	float MagicValueBeforeAttributeChange;
	float MaxMagicValueBeforeAttributeChange;

	// Meta attributes are consumed during GameplayEffect execution and are not replicated.
	UPROPERTY(BlueprintReadOnly, Category = "MazeRunner|Magic", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData MagicConsume;

	UPROPERTY(BlueprintReadOnly, Category = "MazeRunner|Magic", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MagicRecovery;
};
