#include "Weapon/MagicWeaponStateComponent.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Magic/MagicCircleViewActor.h"
#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MagicWeaponStateComponent)

UMagicWeaponStateComponent::UMagicWeaponStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMagicWeaponStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMagicWeaponStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyAllUnconfirmedPredictiveViewActors();
	Super::EndPlay(EndPlayReason);
}

void UMagicWeaponStateComponent::ClientConfirmTargetData_Implementation(uint16 UniqueId, bool bIsTargetDataValid)
{
	(void)bIsTargetDataValid;
	DestroyUnconfirmedPredictiveViewActor(UniqueId);
}

uint16 UMagicWeaponStateComponent::AllocatePredictiveViewId()
{
	for (uint32 Attempt = 0; Attempt <= MAX_uint16; ++Attempt)
	{
		const uint16 CandidateId = NextPredictiveViewId++;
		const bool bAlreadyInUse = UnconfirmedPredictiveViewActors.ContainsByPredicate(
			[CandidateId](const FPredictiveMagicCircleViewActor& Entry)
			{
				return Entry.UniqueId == CandidateId;
			});

		if (!bAlreadyInUse)
		{
			return CandidateId;
		}
	}

	UE_LOG(LogMalogic, Error, TEXT("MagicWeaponStateComponent [%s] exhausted all predictive view IDs."), *GetNameSafe(this));
	return NextPredictiveViewId++;
}

void UMagicWeaponStateComponent::AddUnconfirmedPredictiveViewActor(const FGameplayAbilityTargetDataHandle& InTargetData, TSubclassOf<AMagicCircleViewActor> ViewActorClass, float PredictedBuildingTime)
{
	if (!ViewActorClass || InTargetData.Num() != 1)
	{
		return;
	}

	AController* Controller = GetController<AController>();
	check(Controller);
	APawn* Pawn = Controller->GetPawn();
	check(Pawn);
	if (!Controller->IsLocalController())
	{
		return;
	}

	const FGameplayAbilityTargetData* TargetData = InTargetData.Get(0);
	if (!TargetData || !TargetData->HasEndPoint())
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicWeaponStateComponent [%s] received target data without a deployment endpoint."), *GetNameSafe(this));
		return;
	}

	const uint16 UniqueId = InTargetData.UniqueId;
	//如果找不到匹配的Id，返回并警告
	if (UnconfirmedPredictiveViewActors.ContainsByPredicate(
		[UniqueId](const FPredictiveMagicCircleViewActor& Entry)
		{
			return Entry.UniqueId == UniqueId;
		}))
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicWeaponStateComponent [%s] rejected duplicate predictive view ID [%u]."), *GetNameSafe(this), UniqueId);
		return;
	}

	UWorld* World = GetWorld();
	const FTransform DeploymentTransform = TargetData->GetEndPointTransform();
	if (!World || DeploymentTransform.ContainsNaN())
	{
		return;
	}

	AMagicCircleViewActor* NewViewActor = World->SpawnActorDeferred<AMagicCircleViewActor>(
		ViewActorClass,
		DeploymentTransform,
		Pawn,
		Pawn,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	check(NewViewActor);

	NewViewActor->InitializePredictedBuilding(PredictedBuildingTime);
	NewViewActor->FinishSpawning(DeploymentTransform);

	FPredictiveMagicCircleViewActor& NewEntry = UnconfirmedPredictiveViewActors.Emplace_GetRef(UniqueId);
	NewEntry.ViewActor = NewViewActor;
}

void UMagicWeaponStateComponent::DestroyUnconfirmedPredictiveViewActor(uint16 UniqueId)
{
	for (int32 EntryIndex = 0; EntryIndex < UnconfirmedPredictiveViewActors.Num(); ++EntryIndex)
	{
		FPredictiveMagicCircleViewActor& Entry = UnconfirmedPredictiveViewActors[EntryIndex];
		if (Entry.UniqueId != UniqueId)
		{
			continue;
		}

		if (AMagicCircleViewActor* ViewActor = Entry.ViewActor.Get())
		{
			ViewActor->Destroy();
		}
		UnconfirmedPredictiveViewActors.RemoveAtSwap(EntryIndex);
		return;
	}
}

void UMagicWeaponStateComponent::DestroyAllUnconfirmedPredictiveViewActors()
{
	for (FPredictiveMagicCircleViewActor& Entry : UnconfirmedPredictiveViewActors)
	{
		if (AMagicCircleViewActor* ViewActor = Entry.ViewActor.Get())
		{
			ViewActor->Destroy();
		}
	}
	UnconfirmedPredictiveViewActors.Reset();
}

