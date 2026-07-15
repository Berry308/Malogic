// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MalogicPawnData.generated.h"

class APawn;
class UAbilitySet;
class UAbilityTagRelationshipMapping;
class UMalogicInputConfig;

/**
 * Immutable configuration shared by a player's persistent state and current pawn.
 */
UCLASS(BlueprintType, Const)
class MALOGIC_API UMalogicPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Pawn class spawned by the game mode for a controller using this data.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|Pawn")
	TSubclassOf<APawn> PawnClass;

	// Ability sets granted once to the PlayerState-owned ability system.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|Abilities")
	TArray<TObjectPtr<UAbilitySet>> AbilitySets;

	// Ability tag relationships applied while a pawn is the ASC avatar.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|Abilities")
	TObjectPtr<UAbilityTagRelationshipMapping> TagRelationshipMapping;

	// Input actions bound by the locally controlled pawn.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|Input")
	TObjectPtr<UMalogicInputConfig> InputConfig;
};
