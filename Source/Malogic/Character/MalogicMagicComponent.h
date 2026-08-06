#pragma once

#include "Components/PawnComponent.h"

#include "MalogicMagicComponent.generated.h"

class AActor;
class UMalogicAbilitySystemComponent;
class UMalogicMagicComponent;
class UMalogicMagicSet;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMalogicMagicAttributeChanged, UMalogicMagicComponent*, MagicComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/** Bridges MagicSet attributes and native GAS events to pawn-facing Blueprint APIs. */
UCLASS(BlueprintType, Meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMalogicMagicComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UMalogicMagicComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Malogic|Magic")
	static UMalogicMagicComponent* FindMagicComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UMalogicMagicComponent>() : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Malogic|Magic")
	void InitializeWithAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent);

	UFUNCTION(BlueprintCallable, Category = "Malogic|Magic")
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "Malogic|Magic")
	float GetMagicValue() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Magic")
	float GetMaxMagicValue() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Magic")
	float GetMagicValueNormalized() const;

	UPROPERTY(BlueprintAssignable, Category = "Malogic|Magic")
	FMalogicMagicAttributeChanged OnMagicValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Malogic|Magic")
	FMalogicMagicAttributeChanged OnMaxMagicValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "Malogic|Magic")
	FMalogicMagicAttributeChanged OnOutOfMagic;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	void TryInitializeWithAbilitySystem();
	virtual void HandleMagicValueChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxMagicValueChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfMagic(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);

	UPROPERTY()
	TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UMalogicMagicSet> MagicSet;
};
