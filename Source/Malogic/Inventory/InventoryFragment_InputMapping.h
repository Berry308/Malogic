#pragma once

#include "Character/MalogicHeroComponent.h"
#include "Inventory/MalogicInventoryItemDefinition.h"

#include "InventoryFragment_InputMapping.generated.h"

UCLASS()
class MALOGIC_API UInventoryFragment_InputMapping : public UMalogicInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Input")
	FInputMappingContextAndPriority InputMappingContext;
};
