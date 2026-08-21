// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "MagicWeaponStateComponent.generated.h"

class AMagicCircleViewActor;
struct FGameplayAbilityTargetDataHandle;

struct FPredictiveMagicCircleViewActor
{
	FPredictiveMagicCircleViewActor() = default;

	explicit FPredictiveMagicCircleViewActor(uint16 InUniqueId)
		: UniqueId(InUniqueId)
	{
	}

	TWeakObjectPtr<AMagicCircleViewActor> ViewActor;
	uint16 UniqueId = 0;
	float SpawnServerTime = 0.0f; //当ViewActor生成时服务器的时间
};

/*
* UMagicWeaponStateComponent
 *
 * 在编辑器中通过Controller蓝图子类中添加组件
 * This component is responsible for managing the state of magic weapons in the game.
 * It handles predictive view actors for magic circles and communicates with the client to confirm target data.
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMagicWeaponStateComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UMagicWeaponStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Rejected deployments destroy the predicted view immediately. Successful views remain until the replicated instance consumes them.
	UFUNCTION(Client, Reliable)
	void ClientConfirmTargetData(uint16 UniqueId, bool bIsTargetDataValid);

	uint16 AllocatePredictiveViewId();
	void AddUnconfirmedPredictiveViewActor(const FGameplayAbilityTargetDataHandle& InTargetData, TSubclassOf<AMagicCircleViewActor> ViewActorClass, float PredictedBuildingTime);
	bool ConsumePredictiveViewActor(uint16 UniqueId, float& OutBuildingProgress);

	UFUNCTION(BlueprintPure, Category = "Magic Weapon")
	int32 GetUnconfirmedPredictiveViewActorCount() const { return UnconfirmedPredictiveViewActors.Num(); }

private:
	void DestroyUnconfirmedPredictiveViewActor(uint16 UniqueId);
	void DestroyAllUnconfirmedPredictiveViewActors();

	TArray<FPredictiveMagicCircleViewActor> UnconfirmedPredictiveViewActors;
	uint16 NextPredictiveViewId = 0;
};
