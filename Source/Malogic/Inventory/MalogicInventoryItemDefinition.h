// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "MalogicInventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class UMalogicInventoryItemInstance;
struct FFrame;

UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UMalogicInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(UMalogicInventoryItemInstance* Instance) const {}
};

UCLASS(Blueprintable, Const, Abstract)
class UMalogicInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UMalogicInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UMalogicInventoryItemFragment>> Fragments;

public:
	const UMalogicInventoryItemFragment* FindFragmentByClass(TSubclassOf<UMalogicInventoryItemFragment> FragmentClass) const;
};

UCLASS()
class ULyraInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UMalogicInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UMalogicInventoryItemDefinition> ItemDef, TSubclassOf<UMalogicInventoryItemFragment> FragmentClass);
};
