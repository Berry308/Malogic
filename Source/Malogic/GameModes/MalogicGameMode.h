// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "MalogicGameMode.generated.h"

class APlayerController;
class AController;
class AActor;
class UMalogicPawnData;

/**
 * Server-owned game rules, including the default player PawnData configured in the editor.
 */
UCLASS()
class MALOGIC_API AMalogicGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	AMalogicGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual bool ShouldSpawnAtStartSpot(AController* Player) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;

	/** Schedules a server-authoritative player restart after the current pawn is detached. */
	void RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset = false);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|Pawn")
	TObjectPtr<const UMalogicPawnData> DefaultPawnData;
};
