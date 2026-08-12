// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicCircleViewActor.generated.h"

UCLASS(Blueprintable)
class MALOGIC_API AMagicCircleViewActor : public AActor
{
	GENERATED_BODY()

public:
	AMagicCircleViewActor();

	UFUNCTION(BlueprintCallable, Category = "Magic Circle")
	void InitializePredictedBuilding(float InActualBuildingTime);

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	float GetActualBuildingTime() const { return ActualBuildingTime; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	float GetBaseBuildingTime() const { return BaseBuildingTime; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle", DisplayName = "On Initialize Predicted Building")
	void K2_OnInitializePredictedBuilding(float InActualBuildingTime);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	float ActualBuildingTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float BaseBuildingTime = 1.0f;
};
