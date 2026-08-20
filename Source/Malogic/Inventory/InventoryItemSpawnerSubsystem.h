// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/MalogicInventoryItemDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "InventoryItemSpawnerSubsystem.generated.h"

class APlayerController;
class UMalogicInventoryItemInstance;

/** Creates inventory instances and distributes them to player QuickBars. */
UCLASS()
class MALOGIC_API UInventoryItemSpawnerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="TagToInit"))
	UMalogicInventoryItemInstance* CreateItemInstanceFromDefinition(
		TSubclassOf<UMalogicInventoryItemDefinition> ItemDef,
		const TArray<FGameplayTag>& TagToInit,
		UObject* Outer = nullptr);

	UFUNCTION(BlueprintCallable)
	void GiveToPlayerQuickBar(APlayerController* Player, UMalogicInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable)
	void GiveToAllPlayerQuickBar(UMalogicInventoryItemInstance* Item);
};
