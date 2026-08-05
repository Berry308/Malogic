// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"

#include "MalogicGameplayAbility_FromEquipment.generated.h"

class UMalogicEquipmentInstance;
class UMalogicInventoryItemInstance;

/** An ability granted by and associated with an equipment instance. */
UCLASS()
class UMalogicGameplayAbility_FromEquipment : public UMalogicGameplayAbility
{
	GENERATED_BODY()

public:
	UMalogicGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category="Lyra|Ability")
	UMalogicEquipmentInstance* GetAssociatedEquipment() const;

	UFUNCTION(BlueprintCallable, Category="Lyra|Ability")
	UMalogicInventoryItemInstance* GetAssociatedItem() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
