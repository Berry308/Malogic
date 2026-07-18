// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicEquipmentManagerComponent.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MalogicLogChannels.h"
#include "Engine/ActorChannel.h"
#include "MalogicEquipmentDefinition.h"
#include "MalogicEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicEquipmentManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

FString FMalogicAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(EquipmentDefinition.Get()));
}

void FMalogicEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FMalogicAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FMalogicEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FMalogicAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->OnEquipped();
		}
	}
}

void FMalogicEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
}

UMalogicAbilitySystemComponent* FMalogicEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UMalogicAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UMalogicEquipmentInstance* FMalogicEquipmentList::AddEntry(TSubclassOf<UMalogicEquipmentDefinition> EquipmentDefinition)
{
	UMalogicEquipmentInstance* Result = nullptr;
	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	const UMalogicEquipmentDefinition* EquipmentCDO = GetDefault<UMalogicEquipmentDefinition>(EquipmentDefinition);
	TSubclassOf<UMalogicEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UMalogicEquipmentInstance::StaticClass();
	}

	FMalogicAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewObject<UMalogicEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	Result = NewEntry.Instance;

	if (UMalogicAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TObjectPtr<const UAbilitySet>& AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, Result);
		}
	}
	else
	{
		UE_LOG(LogMalogic, Warning, TEXT("%s has no AbilitySystemComponent"), *OwnerComponent->GetOwner()->GetName());
	}

	Result->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);
	MarkItemDirty(NewEntry);
	return Result;
}

void FMalogicEquipmentList::RemoveEntry(UMalogicEquipmentInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FMalogicAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			if (UMalogicAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}
			Instance->DestroyEquipmentActors();
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

UMalogicEquipmentManagerComponent::UMalogicEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
}

void UMalogicEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, EquipmentList);
}

UMalogicEquipmentInstance* UMalogicEquipmentManagerComponent::EquipItem(TSubclassOf<UMalogicEquipmentDefinition> EquipmentClass)
{
	UMalogicEquipmentInstance* Result = nullptr;
	if (EquipmentClass != nullptr)
	{
		Result = EquipmentList.AddEntry(EquipmentClass);
		if (Result != nullptr)
		{
			Result->OnEquipped();
			if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
			{
				AddReplicatedSubObject(Result);
			}
		}
	}
	return Result;
}

void UMalogicEquipmentManagerComponent::UnequipItem(UMalogicEquipmentInstance* ItemInstance)
{
	if (ItemInstance != nullptr)
	{
		if (IsUsingRegisteredSubObjectList())
		{
			RemoveReplicatedSubObject(ItemInstance);
		}
		ItemInstance->OnUnequipped();
		EquipmentList.RemoveEntry(ItemInstance);
	}
}

bool UMalogicEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (FMalogicAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UMalogicEquipmentInstance* Instance = Entry.Instance)
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}
	return WroteSomething;
}

void UMalogicEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UMalogicEquipmentManagerComponent::UninitializeComponent()
{
	TArray<UMalogicEquipmentInstance*> AllEquipmentInstances;
	for (const FMalogicAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}
	for (UMalogicEquipmentInstance* EquipInstance : AllEquipmentInstances)
	{
		UnequipItem(EquipInstance);
	}
	Super::UninitializeComponent();
}

void UMalogicEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FMalogicAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (UMalogicEquipmentInstance* Instance = Entry.Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

UMalogicEquipmentInstance* UMalogicEquipmentManagerComponent::GetFirstInstanceOfType(TSubclassOf<UMalogicEquipmentInstance> InstanceType)
{
	for (FMalogicAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UMalogicEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType)) return Instance;
		}
	}
	return nullptr;
}

TArray<UMalogicEquipmentInstance*> UMalogicEquipmentManagerComponent::GetEquipmentInstancesOfType(TSubclassOf<UMalogicEquipmentInstance> InstanceType) const
{
	TArray<UMalogicEquipmentInstance*> Results;
	for (const FMalogicAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (UMalogicEquipmentInstance* Instance = Entry.Instance)
		{
			if (Instance->IsA(InstanceType)) Results.Add(Instance);
		}
	}
	return Results;
}
