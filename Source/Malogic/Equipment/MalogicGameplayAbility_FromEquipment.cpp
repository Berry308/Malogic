// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicGameplayAbility_FromEquipment.h"

#include "MalogicEquipmentInstance.h"
#include "Inventory/MalogicInventoryItemInstance.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGameplayAbility_FromEquipment)

UMalogicGameplayAbility_FromEquipment::UMalogicGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UMalogicEquipmentInstance* UMalogicGameplayAbility_FromEquipment::GetAssociatedEquipment() const
{
	if (FGameplayAbilitySpec* Spec = UGameplayAbility::GetCurrentAbilitySpec())
	{
		return Cast<UMalogicEquipmentInstance>(Spec->SourceObject.Get());
	}

	return nullptr;
}

UMalogicInventoryItemInstance* UMalogicGameplayAbility_FromEquipment::GetAssociatedItem() const
{
	if (UMalogicEquipmentInstance* Equipment = GetAssociatedEquipment())
	{
		return Cast<UMalogicInventoryItemInstance>(Equipment->GetInstigator());
	}
	return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UMalogicGameplayAbility_FromEquipment::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

PRAGMA_DISABLE_DEPRECATION_WARNINGS
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::NonInstanced)
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	{
		Context.AddError(NSLOCTEXT("Lyra", "EquipmentAbilityMustBeInstanced", "Equipment ability must be instanced"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif
