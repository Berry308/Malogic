// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/AbilitySet.h"
#include "Character/MalogicHeroComponent.h"
#include "UObject/NoExportTypes.h"
#include "MalogicMagicCircleDefinition.generated.h"

class AMalogicMagicCircleInstance;

UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class MALOGIC_API UMalogicMagicCircleDefinition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle")
	TSubclassOf<AMalogicMagicCircleInstance> MagicCircleToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	FInputMappingContextAndPriority InputMapping;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle")
	bool bIsPreDeploy = false;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle")
	bool bIsLifetimeFollowInstigator = false;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (ClampMin = "0.0"))
	float BaseBuildingTime = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle")
	TSubclassOf<AActor> PreviewActor;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (ClampMin = "0.0"))
	float DefaultDistance = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Sets")
	TObjectPtr<const UAbilitySet> AbilitySetForPlayer;

	UPROPERTY(EditDefaultsOnly, Category = "Ability Sets")
	TObjectPtr<const UAbilitySet> AbilitySetForMagicCircle;
};
