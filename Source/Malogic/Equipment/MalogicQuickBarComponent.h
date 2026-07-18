// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "Inventory/MalogicInventoryItemInstance.h"

#include "MalogicQuickBarComponent.generated.h"

class AActor;
class UMalogicEquipmentInstance;
class UMalogicEquipmentManagerComponent;

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class UMalogicQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UMalogicQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category="Lyra")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Lyra")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UMalogicInventoryItemInstance*> GetSlots() const { return Slots; }

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UMalogicInventoryItemInstance* GetActiveSlotItem() const;

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, UMalogicInventoryItemInstance* Item);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UMalogicInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

	virtual void BeginPlay() override;

private:
	void UnequipItemInSlot();
	void EquipItemInSlot();
	UMalogicEquipmentManagerComponent* FindEquipmentManager() const;

protected:
	UPROPERTY()
	int32 NumSlots = 3;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<TObjectPtr<UMalogicInventoryItemInstance>> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;

	UPROPERTY()
	TObjectPtr<UMalogicEquipmentInstance> EquippedItem;
};

//USTRUCT(BlueprintType)
//struct FMalogicQuickBarSlotsChangedMessage
//{
//	GENERATED_BODY()
//	UPROPERTY(BlueprintReadOnly, Category=Inventory)
//	TObjectPtr<AActor> Owner = nullptr;
//	UPROPERTY(BlueprintReadOnly, Category=Inventory)
//	TArray<TObjectPtr<UMalogicInventoryItemInstance>> Slots;
//};
//
//USTRUCT(BlueprintType)
//struct FMalogicQuickBarActiveIndexChangedMessage
//{
//	GENERATED_BODY()
//	UPROPERTY(BlueprintReadOnly, Category=Inventory)
//	TObjectPtr<AActor> Owner = nullptr;
//	UPROPERTY(BlueprintReadOnly, Category=Inventory)
//	int32 ActiveIndex = 0;
//};
