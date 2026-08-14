// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Magic/MalogicGA_MagicCircleDeploy.h"
#include "MalogicGA_RotateToRaycastTarget.generated.h"

/**
 * 
 */
UCLASS()
class MALOGIC_API UMalogicGA_RotateToRaycastTarget : public UMalogicGA_MagicCircleDeploy
{
	GENERATED_BODY()

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CalculateDeployTransform(const FGameplayAbilityActorInfo* ActorInfo, FTransform& OutDeployTransform) const override;
};
