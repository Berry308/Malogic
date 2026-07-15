// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameState.h"
#include "MalogicGameState.generated.h"

class UMalogicPlayerSpawningManagerComponent;

/**
 * Match-scoped state that owns server-side player spawn selection.
 */
UCLASS()
class MALOGIC_API AMalogicGameState : public AModularGameStateBase
{
	GENERATED_BODY()

public:
	AMalogicGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UMalogicPlayerSpawningManagerComponent* GetPlayerSpawningManagerComponent() const { return PlayerSpawningManagerComponent; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Malogic|Spawning")
	TObjectPtr<UMalogicPlayerSpawningManagerComponent> PlayerSpawningManagerComponent;
};
