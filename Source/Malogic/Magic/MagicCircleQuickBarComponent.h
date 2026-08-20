// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "MagicCircleQuickBarComponent.generated.h"

class UMagicCircleManagerComponent;
class UMalogicMagicCircleDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMagicCircleQuickBarSlotsChanged, const TArray<TSubclassOf<UMalogicMagicCircleDefinition>>&, Slots);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMagicCircleQuickBarActiveSlotChanged, int32, ActiveSlotIndex);

/** Replicated magic circle definition shortcuts owned by a player controller. */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMagicCircleQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UMagicCircleQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Magic Circle")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category = "Magic Circle")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Magic Circle")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	TArray<TSubclassOf<UMalogicMagicCircleDefinition>> GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	TSubclassOf<UMalogicMagicCircleDefinition> GetActiveSlotMagicCircle() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Magic Circle")
	void AddToSlot(int32 SlotIndex, TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Magic Circle")
	TSubclassOf<UMalogicMagicCircleDefinition> RemoveFromSlot(int32 SlotIndex);

	UPROPERTY(BlueprintAssignable, Category = "Magic Circle")
	FMagicCircleQuickBarSlotsChanged OnSlotsChanged;

	UPROPERTY(BlueprintAssignable, Category = "Magic Circle")
	FMagicCircleQuickBarActiveSlotChanged OnActiveSlotIndexChanged;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void EquipMagicCircleInSlot();
	void UnequipMagicCircleInSlot();
	UMagicCircleManagerComponent* FindMagicCircleManager() const;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (ClampMin = "1"))
	int32 NumSlots = 3;

	/** Magic circle definitions assigned to the quick bar when the component starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle")
	TArray<TSubclassOf<UMalogicMagicCircleDefinition>> DefaultSlots;

	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<TSubclassOf<UMalogicMagicCircleDefinition>> Slots;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = INDEX_NONE;
};
