// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "MalogicGameData.generated.h"

class UGameplayEffect;
class UObject;
/**
 * UMalogicGameData
 *
 * Non-mutable data asset that contains global game data.
 * 主要包含三种GameplayEffect
 */
UCLASS()
class MALOGIC_API UMalogicGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UMalogicGameData();

	// Returns the loaded game data.通过LyraAssetManager获取的
	static  const UMalogicGameData& Get();

public:

	// Gameplay effect used to apply damage.  Uses SetByCaller for the damage magnitude.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	// Gameplay effect used to apply healing.  Uses SetByCaller for the healing magnitude.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	// Gameplay effect used to add and remove dynamic tags.
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;

};
