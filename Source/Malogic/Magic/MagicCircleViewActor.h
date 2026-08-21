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
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Magic Circle")
	void InitializePredictedBuilding(float InActualBuildingTime);

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	float GetActualBuildingTime() const { return ActualBuildingTime; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	float GetBaseBuildingTime() const { return BaseBuildingTime; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	float GetCurrentBuildingProgress() const { return CurrentBuildingProgress; }

protected:
	//注意：调用该函数时，实例还未完成FinishSpawning
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle", DisplayName = "On Initialize Predicted Building")
	void K2_OnInitializePredictedBuilding(float InActualBuildingTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle", DisplayName = "Apply Building Progress")
	void K2_ApplyBuildingProgress(float NormalizedProgress);

	//预计的服务器单次传输延迟时间
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ServerDelayTime = 0.15f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	float ActualBuildingTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	float CurrentBuildingProgress = 0.0f;

	float CurrentBuildingTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float BaseBuildingTime = 1.0f;
};
