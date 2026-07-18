// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/AbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "MalogicEquipmentManagerComponent.generated.h"

class UActorComponent;
class UMalogicAbilitySystemComponent;
class UMalogicEquipmentDefinition;
class UMalogicEquipmentInstance;
class UMalogicEquipmentManagerComponent;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

USTRUCT(BlueprintType)
struct FMalogicAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FMalogicAppliedEquipmentEntry() {}
	FString GetDebugString() const;

private:
	friend FMalogicEquipmentList;
	friend UMalogicEquipmentManagerComponent;

	UPROPERTY()
	TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition;

	UPROPERTY()
	TObjectPtr<UMalogicEquipmentInstance> Instance = nullptr;

	UPROPERTY(NotReplicated)
	FAbilitySet_GrantedHandles GrantedHandles;
};

USTRUCT(BlueprintType)
struct FMalogicEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FMalogicEquipmentList() : OwnerComponent(nullptr) {}
	FMalogicEquipmentList(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMalogicAppliedEquipmentEntry, FMalogicEquipmentList>(Entries, DeltaParms, *this);
	}

	UMalogicEquipmentInstance* AddEntry(TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition);
	void RemoveEntry(UMalogicEquipmentInstance* Instance);

private:
	UMalogicAbilitySystemComponent* GetAbilitySystemComponent() const;
	friend UMalogicEquipmentManagerComponent;

	UPROPERTY()
	TArray<FMalogicAppliedEquipmentEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template<>
struct TStructOpsTypeTraits<FMalogicEquipmentList> : public TStructOpsTypeTraitsBase2<FMalogicEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(MinimalAPI, BlueprintType, Const, meta = (BlueprintSpawnableComponent))
class UMalogicEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UMalogicEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UMalogicEquipmentInstance* EquipItem(TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UnequipItem(UMalogicEquipmentInstance* ItemInstance);

	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void ReadyForReplication() override;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	UMalogicEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UMalogicEquipmentInstance> InstanceType);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UMalogicEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UMalogicEquipmentInstance> InstanceType) const;

	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

private:
	UPROPERTY(Replicated)
	FMalogicEquipmentList EquipmentList;
};
