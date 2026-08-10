#pragma once

#include "CoreMinimal.h"
#include "Character/MalogicHeroComponent.h"
#include "Weapon/MalogicWeaponInstance.h"
#include "MalogicMagicWeaponInstance.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;

UCLASS(BlueprintType, Blueprintable)
class MALOGIC_API UMalogicMagicWeaponInstance : public UMalogicWeaponInstance
{
	GENERATED_BODY()

public:
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;

	UFUNCTION(BlueprintPure, Category = "Magic Weapon")
	float GetDeployDistanceRatio() const { return DeployDistanceRatio; }

protected:
	virtual void OnInstigatorChanged() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magic Weapon", meta = (ClampMin = "0.0"))
	float DeployDistanceRatio = 1.0f;

private:
	void ApplyInputMapping();
	void RemoveInputMapping();

	FInputMappingContextAndPriority AppliedInputMapping;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedInputSubsystem;
	TWeakObjectPtr<UInputMappingContext> AppliedInputMappingContext;
	bool bHasAppliedInputMapping = false;
	bool bIsEquipped = false;
};
