// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/World.h"

#include "MalogicEquipmentInstance.generated.h"

class AActor;
class APawn;
class UAnimInstance;
struct FMalogicEquipmentActorToSpawn;

UCLASS(BlueprintType, Blueprintable)
class UMalogicEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UMalogicEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual UWorld* GetWorld() const override final;

	UFUNCTION(BlueprintPure, Category=Equipment)
	UObject* GetInstigator() const { return Instigator; }
	//通常设置为QuickBarComp上某个插槽的UMalogicInventoryItemInstance，Stat Tag Stack在UMalogicInventoryItemInstance上，需要通过Instigator扣除弹药数量
	void SetInstigator(UObject* InInstigator);

	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	//UFUNCTION(BlueprintCallable)
	//TSubclassOf<UAnimInstance> GetFirstPersonAnimInstanceClass() const;

	UFUNCTION(BlueprintCallable)
	TSubclassOf<UAnimInstance> GetThirdPersonAnimInstanceClass() const;

	virtual void SpawnEquipmentActors(const TArray<FMalogicEquipmentActorToSpawn>& ActorsToSpawn);
	virtual void DestroyEquipmentActors();
	virtual void OnEquipped();
	virtual void OnUnequipped();

protected:
	virtual void OnInstigatorChanged() {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

private:
	UFUNCTION()
	void OnRep_Instigator();

	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;
};
