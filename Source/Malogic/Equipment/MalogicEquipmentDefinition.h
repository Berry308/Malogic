// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SubclassOf.h"

#include "MalogicEquipmentDefinition.generated.h"

class AActor;
class UAbilitySet;
class UMalogicEquipmentInstance;

USTRUCT()
struct FMalogicEquipmentActorToSpawn
{
	GENERATED_BODY()

	FMalogicEquipmentActorToSpawn()
	{}

	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};

/**
 * UMalogicEquipmentDefinition
 *
 * Definition of a piece of equipment that can be applied to a pawn
 * EquipmentInstance;AbilitySetsToGrant;FMalogicEquipmentActorToSpawn
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UMalogicEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UMalogicEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<UMalogicEquipmentInstance> InstanceType;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UAbilitySet>> AbilitySetsToGrant;

	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FMalogicEquipmentActorToSpawn> ActorsToSpawn;
};
