// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/AbilitySet.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "MalogicEquipmentManagerComponent.generated.h"

class UActorComponent;
class UObject;
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

	//在客户端从 Items 数组中真正移除元素之前调用。
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	//在客户端向 Items 数组中添加完新元素之后调用。
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	//PostReplicatedChange：在客户端更新了数组中现有元素的属性之后调用。
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		//这是由 Unreal 提供的高度优化的静态函数。它会自动对比当前数组状态与客户端已知的状态，计算出“增量”（Delta），并进行二进制读写。
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

//告诉 Unreal 引擎：“FMalogicEquipmentList包含了一个自定义的 NetDeltaSerialize 函数，请在网络同步时调用它，而不是使用默认的同步逻辑。”
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
	UMalogicEquipmentInstance* EquipItemWithInstigator(TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition, UObject* InInstigator);

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
