#pragma once

#include "Equipment/MalogicEquipmentInstance.h"
#include "MalogicWeaponInstance.generated.h"

UCLASS()
class MALOGIC_API UMalogicWeaponInstance : public UMalogicEquipmentInstance
{
	GENERATED_BODY()

public:
	UMalogicWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnEquipped() override;

	UFUNCTION(BlueprintCallable)
	void UpdateFiringTime();

	UFUNCTION(BlueprintPure)
	float GetTimeSinceLastInteractedWith() const;

private:
	double TimeLastEquipped = 0.0;
	double TimeLastFired = 0.0;
};
