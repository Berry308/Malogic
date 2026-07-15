// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicGameMode.h"

#include "Character/MalogicPawnData.h"
#include "GameModes/MalogicGameState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "MalogicLogChannels.h"
#include "Player/MalogicPlayerSpawningManagerComponent.h"
#include "Player/MalogicPlayerState.h"
#include "TimerManager.h"

AMalogicGameMode::AMalogicGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AMalogicGameState::StaticClass();
}

void AMalogicGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!DefaultPawnData)
	{
		UE_LOG(LogMalogic, Warning, TEXT("GameMode [%s] has no DefaultPawnData configured."), *GetNameSafe(this));
		return;
	}

	if (AMalogicPlayerState* PlayerState = NewPlayer ? NewPlayer->GetPlayerState<AMalogicPlayerState>() : nullptr)
	{
		PlayerState->SetPawnData(DefaultPawnData);
	}
	else
	{
		UE_LOG(LogMalogic, Error, TEXT("Unable to set PawnData [%s] because player [%s] has no MalogicPlayerState."), *GetNameSafe(DefaultPawnData), *GetNameSafe(NewPlayer));
	}
}

UClass* AMalogicGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (InController)
	{
		if (const AMalogicPlayerState* PlayerState = InController->GetPlayerState<AMalogicPlayerState>())
		{
			if (const UMalogicPawnData* PawnData = PlayerState->GetPawnData<UMalogicPawnData>())
			{
				if (PawnData->PawnClass)
				{
					return PawnData->PawnClass;
				}
			}
		}
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

bool AMalogicGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	(void)Player;
	return false;
}

AActor* AMalogicGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (AMalogicGameState* MalogicGameState = Cast<AMalogicGameState>(GameState))
	{
		if (UMalogicPlayerSpawningManagerComponent* SpawningManager = MalogicGameState->GetPlayerSpawningManagerComponent())
		{
			if (AActor* PlayerStart = SpawningManager->ChoosePlayerStart(Player))
			{
				return PlayerStart;
			}
		}
	}

	return Super::ChoosePlayerStart_Implementation(Player);
}

bool AMalogicGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	if (!Super::PlayerCanRestart_Implementation(Player))
	{
		return false;
	}

	if (AMalogicGameState* MalogicGameState = Cast<AMalogicGameState>(GameState))
	{
		if (const UMalogicPlayerSpawningManagerComponent* SpawningManager = MalogicGameState->GetPlayerSpawningManagerComponent())
		{
			return SpawningManager->ControllerCanRestart(Player);
		}
	}

	return true;
}

void AMalogicGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	if (AMalogicGameState* MalogicGameState = Cast<AMalogicGameState>(GameState))
	{
		if (UMalogicPlayerSpawningManagerComponent* SpawningManager = MalogicGameState->GetPlayerSpawningManagerComponent())
		{
			SpawningManager->FinishRestartPlayer(NewPlayer, StartRotation);
		}
	}

	Super::FinishRestartPlayer(NewPlayer, StartRotation);
}

void AMalogicGameMode::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (!Controller)
	{
		return;
	}

	if (bForceReset)
	{
		Controller->Reset();
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		GetWorldTimerManager().SetTimerForNextTick(PlayerController, &APlayerController::ServerRestartPlayer_Implementation);
	}
}
