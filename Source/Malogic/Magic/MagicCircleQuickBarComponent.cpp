#include "Magic/MagicCircleQuickBarComponent.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Magic/MagicCircleManagerComponent.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MagicCircleQuickBarComponent)

UMagicCircleQuickBarComponent::UMagicCircleQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UMagicCircleQuickBarComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Slots = DefaultSlots;
		Slots.SetNum(NumSlots);
	}
}

void UMagicCircleQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UMagicCircleQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 InitialIndex = ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex;
	int32 NewIndex = InitialIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex])
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != InitialIndex);
}

void UMagicCircleQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 InitialIndex = ActiveSlotIndex < 0 ? 0 : ActiveSlotIndex;
	int32 NewIndex = InitialIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex])
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != InitialIndex);
}

void UMagicCircleQuickBarComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || !Slots[NewIndex] || ActiveSlotIndex == NewIndex)
	{
		return;
	}

	UnequipMagicCircleInSlot();
	ActiveSlotIndex = NewIndex;
	EquipMagicCircleInSlot();
	OnRep_ActiveSlotIndex();
	GetOwner()->ForceNetUpdate();
}

TSubclassOf<UMalogicMagicCircleDefinition> UMagicCircleQuickBarComponent::GetActiveSlotMagicCircle() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

void UMagicCircleQuickBarComponent::AddToSlot(int32 SlotIndex, TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (!Slots.IsValidIndex(SlotIndex) || !MagicCircleDefinition || Slots[SlotIndex] == MagicCircleDefinition)
	{
		return;
	}

	if (Slots[SlotIndex] == nullptr)
	{
		Slots[SlotIndex] = MagicCircleDefinition;
		OnRep_Slots();
		GetOwner()->ForceNetUpdate();
	}
	else
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleQuickBarComponent [%s] could not add a magic circle to occupied slot [%d]."), *GetNameSafe(this), SlotIndex);
	}
}

TSubclassOf<UMalogicMagicCircleDefinition> UMagicCircleQuickBarComponent::RemoveFromSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return nullptr;
	}

	TSubclassOf<UMalogicMagicCircleDefinition> RemovedMagicCircle;
	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipMagicCircleInSlot();
		ActiveSlotIndex = INDEX_NONE;
		OnRep_ActiveSlotIndex();
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		RemovedMagicCircle = Slots[SlotIndex];
		if (RemovedMagicCircle)
		{
			Slots[SlotIndex] = nullptr;
			OnRep_Slots();
			GetOwner()->ForceNetUpdate();
		}
	}

	return RemovedMagicCircle;
}

void UMagicCircleQuickBarComponent::EquipMagicCircleInSlot()
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return;
	}

	if (const TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition = Slots[ActiveSlotIndex])
	{
		if (UMagicCircleManagerComponent* MagicCircleManager = FindMagicCircleManager())
		{
			MagicCircleManager->EquipMagicCircle(MagicCircleDefinition);
		}
	}
}

void UMagicCircleQuickBarComponent::UnequipMagicCircleInSlot()
{
	if (UMagicCircleManagerComponent* MagicCircleManager = FindMagicCircleManager())
	{
		MagicCircleManager->UnequipMagicCircle();
	}
}

UMagicCircleManagerComponent* UMagicCircleQuickBarComponent::FindMagicCircleManager() const
{
	if (const AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UMagicCircleManagerComponent>();
		}
	}

	return nullptr;
}

void UMagicCircleQuickBarComponent::OnRep_Slots()
{
	OnSlotsChanged.Broadcast(Slots);
}

void UMagicCircleQuickBarComponent::OnRep_ActiveSlotIndex()
{
	OnActiveSlotIndexChanged.Broadcast(ActiveSlotIndex);
}

