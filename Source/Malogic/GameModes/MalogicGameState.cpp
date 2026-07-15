// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/MalogicGameState.h"

#include "Player/MalogicPlayerSpawningManagerComponent.h"

AMalogicGameState::AMalogicGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerSpawningManagerComponent = CreateDefaultSubobject<UMalogicPlayerSpawningManagerComponent>(TEXT("PlayerSpawningManagerComponent"));
}
