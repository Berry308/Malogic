// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameStateComponent.h"

#include "MalogicPlayerSpawningManagerComponent.generated.h"

class AActor;
class AController;
class APlayerStart;
class AMalogicPlayerStart;
class ULevel;
class UWorld;

/** Server-side cache and selection policy for player spawn points in the current world. */
UCLASS()
class MALOGIC_API UMalogicPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UMalogicPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;

	AActor* ChoosePlayerStart(AController* Player);
	bool ControllerCanRestart(AController* Player) const;
	void FinishRestartPlayer(AController* Player, const FRotator& StartRotation);

protected:
	//自定义生成逻辑
	virtual AActor* OnChoosePlayerStart(AController* Player, TArray<AMalogicPlayerStart*>& PlayerStarts) { return nullptr; }
	//自定义生成结束逻辑
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) {}

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnFinishRestartPlayer"))
	void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);


private:
	APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AMalogicPlayerStart*>& StartPoints) const;
	void OnLevelAdded(ULevel* Level, UWorld* World);
	void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	APlayerStart* FindPlayFromHereStart(AController* Player) const;
#endif

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AMalogicPlayerStart>> CachedPlayerStarts;

	FDelegateHandle ActorSpawnedHandle;
};
