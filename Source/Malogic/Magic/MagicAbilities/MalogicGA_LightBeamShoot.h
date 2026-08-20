// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Magic/MalogicGA_FromMagicCircle.h"
#include "MalogicGA_LightBeamShoot.generated.h"

/**
 * 
 */
UCLASS()
class MALOGIC_API UMalogicGA_LightBeamShoot : public UMalogicGA_FromMagicCircle
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Magic|Light Beam")
	void K2_OnMagicHit(const FHitResult& HitResult);
};
