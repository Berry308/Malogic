// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkComponent.h"

#include "MalogicHealthComponent.generated.h"

class UMalogicAbilitySystemComponent;
class UMalogicHealthComponent;
class UMalogicHealthSet;
class AActor;
class FLifetimeProperty;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMalogicHealthDeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMalogicHealthAttributeChanged, UMalogicHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

UENUM(BlueprintType)
enum class EMalogicDeathState : uint8
{
	NotDead = 0,
	DeathStarted,
	DeathFinished
};

/** Handles health attributes, death state, and the death gameplay event for an actor. */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMalogicHealthComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()

public:
	UMalogicHealthComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintPure, Category = "Malogic|Health")
	static UMalogicHealthComponent* FindHealthComponent(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UMalogicHealthComponent>() : nullptr;
	}

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	void InitializeWithAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent);

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	void UninitializeFromAbilitySystem();

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	float GetHealthNormalized() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Health")
	EMalogicDeathState GetDeathState() const { return DeathState; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Malogic|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return DeathState > EMalogicDeathState::NotDead; }

	virtual void StartDeath();
	virtual void FinishDeath();
	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);

	UPROPERTY(BlueprintAssignable)
	FMalogicHealthAttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FMalogicHealthAttributeChanged OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FMalogicHealthDeathEvent OnDeathStarted;

	UPROPERTY(BlueprintAssignable)
	FMalogicHealthDeathEvent OnDeathFinished;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnUnregister() override;

	void ClearGameplayTags();
	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	UFUNCTION()
	virtual void OnRep_DeathState(EMalogicDeathState OldDeathState);

	UPROPERTY()
	TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const UMalogicHealthSet> HealthSet;

	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EMalogicDeathState DeathState;
};
