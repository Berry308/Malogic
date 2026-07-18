// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Inventory/MalogicInventoryItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "InventoryFragment_EquippableItem.generated.h"

class UMalogicEquipmentDefinition;
class UObject;

UCLASS()
class UInventoryFragment_EquippableItem : public UMalogicInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Lyra)
	TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition;
};
