// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicQuickBarComponent.h"

#include "Equipment/MalogicEquipmentDefinition.h"
#include "Equipment/MalogicEquipmentInstance.h"
#include "Equipment/MalogicEquipmentManagerComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryFragment_EquippableItem.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicQuickBarComponent)

class FLifetimeProperty;


UMalogicQuickBarComponent::UMalogicQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	
}

void UMalogicQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UMalogicQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots) Slots.AddDefaulted(NumSlots - Slots.Num());
	Super::BeginPlay();
}

void UMalogicQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2) return;
	const int32 OldIndex = ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex;
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr) { SetActiveSlotIndex(NewIndex); return; }
	} while (NewIndex != OldIndex);
}

void UMalogicQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2) return;
	const int32 OldIndex = ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex;
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr) { SetActiveSlotIndex(NewIndex); return; }
	} while (NewIndex != OldIndex);
}

void UMalogicQuickBarComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));
	check(EquippedItem == nullptr);
	if (UMalogicInventoryItemInstance* SlotItem = Slots[ActiveSlotIndex])
	{
		if (const UInventoryFragment_EquippableItem* EquipInfo = SlotItem->FindFragmentByClass<UInventoryFragment_EquippableItem>())
		{
			TSubclassOf<UMalogicEquipmentDefinition> EquipDef = EquipInfo->EquipmentDefinition;
			if (EquipDef != nullptr)
			{
				if (UMalogicEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
				{
					EquippedItem = EquipmentManager->EquipItemWithInstigator(EquipDef, SlotItem);
				}
			}
		}
	}
}

void UMalogicQuickBarComponent::UnequipItemInSlot()
{
	if (UMalogicEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		if (EquippedItem != nullptr)
		{
			EquipmentManager->UnequipItem(EquippedItem);
			EquippedItem = nullptr;
		}
	}
}

UMalogicEquipmentManagerComponent* UMalogicQuickBarComponent::FindEquipmentManager() const
{
	if (AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn()) return Pawn->FindComponentByClass<UMalogicEquipmentManagerComponent>();
	}
	return nullptr;
}

void UMalogicQuickBarComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && ActiveSlotIndex != NewIndex)
	{
		UnequipItemInSlot();
		ActiveSlotIndex = NewIndex;
		EquipItemInSlot();
		OnRep_ActiveSlotIndex();
	}
}

UMalogicInventoryItemInstance* UMalogicQuickBarComponent::GetActiveSlotItem() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

int32 UMalogicQuickBarComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (const TObjectPtr<UMalogicInventoryItemInstance>& ItemPtr : Slots)
	{
		if (ItemPtr == nullptr) return SlotIndex;
		++SlotIndex;
	}
	return INDEX_NONE;
}

void UMalogicQuickBarComponent::AddItemToSlot(int32 SlotIndex, UMalogicInventoryItemInstance* Item)
{
	if (Slots.IsValidIndex(SlotIndex) && Item != nullptr)
	{
		if (Slots[SlotIndex] == nullptr) { Slots[SlotIndex] = Item; OnRep_Slots(); }
	}
	else if (!Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s QuickBarComponent AddItemToSlot has failed1"), *GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s QuickBarComponent AddItemToSlot has failed2"), *GetOwner()->GetName());
	}
}

UMalogicInventoryItemInstance* UMalogicQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	UMalogicInventoryItemInstance* Result = nullptr;
	if (ActiveSlotIndex == SlotIndex) { UnequipItemInSlot(); ActiveSlotIndex = -1; }
	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];
		if (Result != nullptr) { Slots[SlotIndex] = nullptr; OnRep_Slots(); }
	}
	return Result;
}

void UMalogicQuickBarComponent::OnRep_Slots()
{
}

void UMalogicQuickBarComponent::OnRep_ActiveSlotIndex()
{
}
