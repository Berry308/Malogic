// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/MalogicPlayerSpawningManagerComponent.h"

#include "Engine/Level.h"
#include "Engine/PlayerStartPIE.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/MalogicPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicPlayerSpawningManagerComponent)

UMalogicPlayerSpawningManagerComponent::UMalogicPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	bWantsInitializeComponent = true;
}

void UMalogicPlayerSpawningManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UWorld* World = GetWorld();
	if (!World || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &ThisClass::OnLevelAdded);
	ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::HandleOnActorSpawned));

	for (TActorIterator<AMalogicPlayerStart> It(World); It; ++It)
	{
		CachedPlayerStarts.Add(*It);
	}
}

void UMalogicPlayerSpawningManagerComponent::UninitializeComponent()
{
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);

	if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		ActorSpawnedHandle.Reset();
	}

	CachedPlayerStarts.Reset();
	Super::UninitializeComponent();
}

AActor* UMalogicPlayerSpawningManagerComponent::ChoosePlayerStart(AController* Player)
{
	if (!Player)
	{
		return nullptr;
	}

#if WITH_EDITOR
	if (APlayerStart* PlayerStart = FindPlayFromHereStart(Player))
	{
		return PlayerStart;
	}
#endif

	TArray<AMalogicPlayerStart*> StartPoints;
	for (auto It = CachedPlayerStarts.CreateIterator(); It; ++It)
	{
		if (AMalogicPlayerStart* Start = It->Get())
		{
			StartPoints.Add(Start);
		}
		else
		{
			It.RemoveCurrent();
		}
	}

	if (const APlayerState* PlayerState = Player->GetPlayerState<APlayerState>(); PlayerState && PlayerState->IsOnlyASpectator())
	{
		return StartPoints.IsEmpty() ? nullptr : StartPoints[FMath::RandRange(0, StartPoints.Num() - 1)];
	}

	//此处可以自定义生成规则
	AActor* PlayerStart = OnChoosePlayerStart(Player, StartPoints);

	if (!PlayerStart)
	{
		PlayerStart = GetFirstRandomUnoccupiedPlayerStart(Player, StartPoints);
	}

	if (AMalogicPlayerStart* Start = Cast<AMalogicPlayerStart>(PlayerStart))
	{
		Start->TryClaim(Player);
	}

	return PlayerStart;

}

bool UMalogicPlayerSpawningManagerComponent::ControllerCanRestart(AController* Player) const
{
	return Player != nullptr;
}

void UMalogicPlayerSpawningManagerComponent::FinishRestartPlayer(AController* Player, const FRotator& StartRotation)
{
	OnFinishRestartPlayer(Player, StartRotation);
	K2_OnFinishRestartPlayer(Player, StartRotation);
}

APlayerStart* UMalogicPlayerSpawningManagerComponent::GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AMalogicPlayerStart*>& StartPoints) const
{
	TArray<AMalogicPlayerStart*> EmptyStarts;
	TArray<AMalogicPlayerStart*> PartialStarts;

	for (AMalogicPlayerStart* StartPoint : StartPoints)
	{
		if (!StartPoint)
		{
			continue;
		}

		switch (StartPoint->GetLocationOccupancy(Controller))
		{
		case EMalogicPlayerStartLocationOccupancy::Empty:
			EmptyStarts.Add(StartPoint);
			break;
		case EMalogicPlayerStartLocationOccupancy::Partial:
			PartialStarts.Add(StartPoint);
			break;
		default:
			break;
		}
	}

	if (!EmptyStarts.IsEmpty())
	{
		return EmptyStarts[FMath::RandRange(0, EmptyStarts.Num() - 1)];
	}

	return PartialStarts.IsEmpty() ? nullptr : PartialStarts[FMath::RandRange(0, PartialStarts.Num() - 1)];
}

void UMalogicPlayerSpawningManagerComponent::OnLevelAdded(ULevel* Level, UWorld* World)
{
	if (World != GetWorld() || !Level)
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		if (AMalogicPlayerStart* PlayerStart = Cast<AMalogicPlayerStart>(Actor))
		{
			CachedPlayerStarts.AddUnique(PlayerStart);
		}
	}
}

void UMalogicPlayerSpawningManagerComponent::HandleOnActorSpawned(AActor* SpawnedActor)
{
	if (AMalogicPlayerStart* PlayerStart = Cast<AMalogicPlayerStart>(SpawnedActor))
	{
		CachedPlayerStarts.AddUnique(PlayerStart);
	}
}

#if WITH_EDITOR
APlayerStart* UMalogicPlayerSpawningManagerComponent::FindPlayFromHereStart(AController* Player) const
{
	if (!Player || !Player->IsA<APlayerController>())
	{
		return nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			if (It->IsA<APlayerStartPIE>())
			{
				return *It;
			}
		}
	}

	return nullptr;
}
#endif
