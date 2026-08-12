// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "MalogicGA_MagicCircleDeploy.generated.h"

class UMalogicMagicCircleDefinition;
class UMalogicMagicWeaponInstance;
class AMalogicMagicCircleInstance;
struct FGameplayAbilityTargetDataHandle;

UCLASS()
class MALOGIC_API UMalogicGA_MagicCircleDeploy : public UMalogicGameplayAbility
{
	GENERATED_BODY()

public:
	UMalogicGA_MagicCircleDeploy(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CalculateDeployTransform(const FGameplayAbilityActorInfo* ActorInfo, FTransform& OutDeployTransform) const;
	virtual float CalculateActualBuildingTime(const UMalogicMagicCircleDefinition* Definition, const FGameplayAbilityActorInfo* ActorInfo) const;

private:
	const UMalogicMagicCircleDefinition* GetAssociatedDefinition(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	UMalogicMagicWeaponInstance* GetMagicWeaponInstance(const FGameplayAbilityActorInfo* ActorInfo) const;
	bool ValidateDeploymentTargetData(const FGameplayAbilityTargetDataHandle& TargetData, FTransform& OutDeployTransform) const;
	AMalogicMagicCircleInstance* SpawnMagicCircleInstance(const UMalogicMagicCircleDefinition* Definition, const FGameplayAbilityActorInfo* ActorInfo, const FTransform& DeployTransform, float ActualBuildingTime) const;

	void StartDeploymentTargeting();
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (ClampMin = "0.01"))
	float MinimumActualBuildingTime = 0.01f;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (ClampMin = "0.01"))
	float MaximumActualBuildingTime = 60.0f;

	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};
