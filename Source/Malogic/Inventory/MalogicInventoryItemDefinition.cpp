// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicInventoryItemDefinition.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicInventoryItemDefinition)

UMalogicInventoryItemDefinition::UMalogicInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const UMalogicInventoryItemFragment* UMalogicInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UMalogicInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UMalogicInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

const UMalogicInventoryItemFragment* UMalogicInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UMalogicInventoryItemDefinition> ItemDef, TSubclassOf<UMalogicInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UMalogicInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}
