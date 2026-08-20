// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/InventoryItemSpawnerSubsystem.h"

#include "Engine/World.h"
#include "Equipment/MalogicQuickBarComponent.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/MalogicInventoryItemDefinition.h"
#include "Inventory/MalogicInventoryItemInstance.h"
#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InventoryItemSpawnerSubsystem)

UMalogicInventoryItemInstance* UInventoryItemSpawnerSubsystem::CreateItemInstanceFromDefinition(
	TSubclassOf<UMalogicInventoryItemDefinition> ItemDef,
	const TArray<FGameplayTag>& TagToInit,
	UObject* Outer)
{
	UMalogicInventoryItemInstance* ItemInstance = NewObject<UMalogicInventoryItemInstance>(Outer);

	if (ItemDef == nullptr)
	{
		UE_LOG(LogMalogic, Warning, TEXT("CreateItemInstanceFromDefinition failed: ItemDef is null."));
		return ItemInstance;
	}

	ItemInstance->SetItemDef(ItemDef);

	for (const FGameplayTag& Tag : TagToInit)
	{
		ItemInstance->AddStatTagStack(Tag, 1);
	}

	return ItemInstance;
}

void UInventoryItemSpawnerSubsystem::GiveToPlayerQuickBar(APlayerController* Player, UMalogicInventoryItemInstance* Item)
{
	if (Player == nullptr || Item == nullptr)
	{
		UE_LOG(LogMalogic, Warning, TEXT("GiveToPlayerQuickBar failed: Player or Item is null."));
		return;
	}

	if (!Player->HasAuthority())
	{
		UE_LOG(LogMalogic, Warning, TEXT("GiveToPlayerQuickBar rejected non-authoritative Player '%s'."), *Player->GetName());
		return;
	}

	if (UMalogicQuickBarComponent* QuickBar = Player->FindComponentByClass<UMalogicQuickBarComponent>())
	{
		const int32 ItemSlot = QuickBar->GetNextFreeItemSlot();
		if (ItemSlot != INDEX_NONE)
		{
			QuickBar->AddItemToSlot(ItemSlot, Item);
			QuickBar->SetActiveSlotIndex(ItemSlot);
			return;
		}
	}

	const UMalogicInventoryItemDefinition* ItemDefinition = Item->GetItemDef()
		? GetDefault<UMalogicInventoryItemDefinition>(Item->GetItemDef())
		: nullptr;
	UE_LOG(LogMalogic, Warning, TEXT("GiveToPlayerQuickBar failed for Player '%s' and Item '%s'."),
		*Player->GetName(),
		ItemDefinition ? *ItemDefinition->GetName() : TEXT("None"));
}

void UInventoryItemSpawnerSubsystem::GiveToAllPlayerQuickBar(UMalogicInventoryItemInstance* Item)
{
	if (Item == nullptr)
	{
		UE_LOG(LogMalogic, Warning, TEXT("GiveToAllPlayerQuickBar failed: Item is null."));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (World == nullptr)
	{
		UE_LOG(LogMalogic, Warning, TEXT("GiveToAllPlayerQuickBar failed: World is null."));
		return;
	}

	if (World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogMalogic, Warning, TEXT("GiveToAllPlayerQuickBar rejected execution on a client world."));
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* Player = It->Get())
		{
			GiveToPlayerQuickBar(Player, Item);
		}
	}
}
