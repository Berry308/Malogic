#include "Magic/MagicCircleViewActor.h"

AMagicCircleViewActor::AMagicCircleViewActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;
	SetReplicateMovement(false);
}

void AMagicCircleViewActor::InitializePredictedBuilding(float InActualBuildingTime)
{
	ActualBuildingTime = FMath::Max(0.0f, InActualBuildingTime + ServerDelayTime);
	CurrentBuildingTime = 0.0f;
	CurrentBuildingProgress = ActualBuildingTime > KINDA_SMALL_NUMBER ? 0.0f : 1.0f;
	K2_OnInitializePredictedBuilding(ActualBuildingTime);
	//K2_ApplyBuildingProgress(CurrentBuildingProgress);此时还没有触发BeginPlay调用创建动态材质。
	SetActorTickEnabled(CurrentBuildingProgress < 1.0f);
}

void AMagicCircleViewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep the progress in C++ so the authoritative actor can take over the exact displayed value.
	CurrentBuildingTime += DeltaSeconds;
	CurrentBuildingProgress = ActualBuildingTime > KINDA_SMALL_NUMBER
		? FMath::Clamp(CurrentBuildingTime / ActualBuildingTime, 0.0f, 1.0f)
		: 1.0f;
	K2_ApplyBuildingProgress(CurrentBuildingProgress);

	if (CurrentBuildingProgress >= 1.0f)
	{
		SetActorTickEnabled(false);
	}
}

