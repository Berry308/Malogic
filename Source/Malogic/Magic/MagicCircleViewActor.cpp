#include "Magic/MagicCircleViewActor.h"

AMagicCircleViewActor::AMagicCircleViewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetReplicateMovement(false);
}

void AMagicCircleViewActor::InitializePredictedBuilding(float InActualBuildingTime)
{
	ActualBuildingTime = FMath::Max(0.0f, InActualBuildingTime);
	K2_OnInitializePredictedBuilding(ActualBuildingTime);
}

