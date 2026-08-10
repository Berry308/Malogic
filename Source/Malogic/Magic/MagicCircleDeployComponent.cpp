#include "Magic/MagicCircleDeployComponent.h"

#include "Character/MalogicHealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Magic/MagicCircleManagerComponent.h"
#include "MalogicLogChannels.h"
#include "Math/RotationMatrix.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MagicCircleDeployComponent)

UMagicCircleDeployComponent::UMagicCircleDeployComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UMagicCircleDeployComponent::OnRegister()
{
	Super::OnRegister();

	if (!GetPawn<APawn>())
	{
		UE_LOG(LogMalogic, Error, TEXT("MagicCircleDeployComponent [%s] must be attached to a Pawn."), *GetNameSafe(this));
		return;
	}

	BindMagicCircleManager();
}

void UMagicCircleDeployComponent::BeginPlay()
{
	Super::BeginPlay();

	BindMagicCircleManager();

	if (UMalogicHealthComponent* HealthComponent = UMalogicHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComponent->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandlePawnDeathStarted);
	}

	if (MagicCircleManager.IsValid())
	{
		HandleMagicCircleDefinitionChanged(MagicCircleManager->GetEquippedMagicCircle());
	}
}

void UMagicCircleDeployComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MagicCircleManager.IsValid())
	{
		MagicCircleManager->OnMagicCircleDefinitionChanged.RemoveAll(this);
	}

	if (UMalogicHealthComponent* HealthComponent = UMalogicHealthComponent::FindHealthComponent(GetOwner()))
	{
		HealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandlePawnDeathStarted);
	}

	ClearPreDeployMagicCircle();
	Super::EndPlay(EndPlayReason);
}

void UMagicCircleDeployComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdatePreDeployMagicCircle(DeltaTime);
}

void UMagicCircleDeployComponent::HandleMagicCirclePreDeploy(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition)
{
	ClearPreDeployMagicCircle();

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled()) return;

	if (!MagicCircleDefinition) return;


	CurrentMagicCircleDefinition = MagicCircleDefinition;

	const UMalogicMagicCircleDefinition* Definition = MagicCircleDefinition->GetDefaultObject<UMalogicMagicCircleDefinition>();
	if (!Definition)
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleDeployComponent [%s] received a definition without a valid CDO."), *GetNameSafe(this));
		CurrentMagicCircleDefinition = nullptr;
		return;
	}

	BaseMaxDeployDistance = FMath::Max(0.0f, Definition->BaseMaxDeployDistance);
	DeployStrategy = Definition->DeployStrategy;
	DistanceFromSource = GetMaxDeployDistance();

	if (!Definition->bIsPreDeploy || !Definition->PreviewActor)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector SourceLocation = Pawn->GetActorLocation();
	FVector SourceDirection = Pawn->GetActorForwardVector();
	GetDeploymentSource(SourceLocation, SourceDirection);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Pawn;
	SpawnParameters.Instigator = Pawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MagicCirclePreview = World->SpawnActor<AActor>(Definition->PreviewActor, SourceLocation, SourceDirection.Rotation(), SpawnParameters);
	if (!MagicCirclePreview)
	{
		UE_LOG(LogMalogic, Warning, TEXT("MagicCircleDeployComponent [%s] failed to spawn preview actor [%s]."), *GetNameSafe(this), *GetNameSafe(Definition->PreviewActor));
		CurrentMagicCircleDefinition = nullptr;
		return;
	}

	MagicCirclePreview->SetReplicates(false);
	MagicCirclePreview->SetReplicateMovement(false);
	MagicCirclePreview->SetActorHiddenInGame(false);
	SetComponentTickEnabled(true);
	UpdatePreDeployMagicCircle(0.0f);
}

void UMagicCircleDeployComponent::UpdatePreDeployMagicCircle(float DeltaTime)
{
	(void)DeltaTime;

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled() || !IsValid(MagicCirclePreview))
	{
		if (!Pawn || !Pawn->IsLocallyControlled())
		{
			ClearPreDeployMagicCircle();
		}
		return;
	}

	FVector SourceLocation;
	FVector SourceDirection;
	if (!GetDeploymentSource(SourceLocation, SourceDirection))
	{
		SetPreviewTransform(FTransform::Identity, false);
		return;
	}

	FTransform DeployTransform;
	const bool bIsValid = CalculateDeployTransform(SourceLocation, SourceDirection, DeployTransform);
	SetPreviewTransform(DeployTransform, bIsValid);
}

void UMagicCircleDeployComponent::IncreaseDeployDistance()
{
	SetAndClampDeployDistance(DistanceFromSource + DeployDistanceStep);
}

void UMagicCircleDeployComponent::DecreaseDeployDistance()
{
	SetAndClampDeployDistance(DistanceFromSource - DeployDistanceStep);
}

void UMagicCircleDeployComponent::SetAndClampDeployDistance(float NewDistance)
{
	DistanceFromSource = FMath::Clamp(NewDistance, 0.0f, GetMaxDeployDistance());
	if (IsValid(MagicCirclePreview))
	{
		UpdatePreDeployMagicCircle(0.0f);
	}
}

void UMagicCircleDeployComponent::ClearPreDeployMagicCircle()
{
	if (IsValid(MagicCirclePreview))
	{
		MagicCirclePreview->Destroy();
	}

	MagicCirclePreview = nullptr;
	CurrentMagicCircleDefinition = nullptr;
	DistanceFromSource = 0.0f;
	BaseMaxDeployDistance = 0.0f;
	DeployStrategy = EMagicCircleDeployStrategy::CameraRaycast;
	bCanBeDeployed = false;
	CurrentDeployTransform = FTransform::Identity;

	SetComponentTickEnabled(false);
}

bool UMagicCircleDeployComponent::GetCurrentDeployTransform(FTransform& OutTransform) const
{
	if (!bCanBeDeployed || !IsValid(MagicCirclePreview))
	{
		return false;
	}

	OutTransform = CurrentDeployTransform;
	return true;
}

void UMagicCircleDeployComponent::SetMaxDeployDistanceRatio(float NewRatio)
{
	MaxDeployDistanceRatio = FMath::Max(0.0f, NewRatio);
	SetAndClampDeployDistance(DistanceFromSource);
}

float UMagicCircleDeployComponent::GetMaxDeployDistance() const
{
	return FMath::Max(0.0f, BaseMaxDeployDistance * FMath::Max(0.0f, MaxDeployDistanceRatio));
}

void UMagicCircleDeployComponent::BindMagicCircleManager()
{
	if (MagicCircleManager.IsValid())
	{
		return;
	}

	if (UMagicCircleManagerComponent* Manager = GetOwner() ? GetOwner()->FindComponentByClass<UMagicCircleManagerComponent>() : nullptr)
	{
		MagicCircleManager = Manager;
		Manager->OnMagicCircleDefinitionChanged.AddUObject(this, &ThisClass::HandleMagicCircleDefinitionChanged);
	}
}

void UMagicCircleDeployComponent::HandleMagicCircleDefinitionChanged(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition)
{
	HandleMagicCirclePreDeploy(MagicCircleDefinition);
}

void UMagicCircleDeployComponent::HandlePawnDeathStarted(AActor* OwningActor)
{
	(void)OwningActor;
	ClearPreDeployMagicCircle();
}

	//当Controller发生变化时，MagicCircleManager作为PawnComponent应该按理说会跟随Pawn被销毁了
bool UMagicCircleDeployComponent::GetDeploymentSource(FVector& OutLocation, FVector& OutDirection) const
{
	const APawn* Pawn = GetPawn<APawn>();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return false;
	}

	if (DeployStrategy == EMagicCircleDeployStrategy::PawnForward)
	{
		OutLocation = Pawn->GetActorLocation();
		OutDirection = Pawn->GetActorForwardVector().GetSafeNormal();
		return !OutDirection.IsNearlyZero();
	}
	else if(DeployStrategy == EMagicCircleDeployStrategy::CameraForward || DeployStrategy == EMagicCircleDeployStrategy::CameraRaycast)
	{
		const APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
		if (!PlayerController || !PlayerController->IsLocalController())
		{
			return false;
		}

		FRotator ViewRotation;
		PlayerController->GetPlayerViewPoint(OutLocation, ViewRotation);
		OutDirection = ViewRotation.Vector().GetSafeNormal();
		return !OutDirection.IsNearlyZero();
	}

	return false;
}

bool UMagicCircleDeployComponent::CalculateDeployTransform(const FVector& SourceLocation, const FVector& SourceDirection, FTransform& OutTransform) const
{
	const FVector Direction = SourceDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	if (DeployStrategy == EMagicCircleDeployStrategy::CameraRaycast)
	{
		UWorld* World = GetWorld();
		const APawn* Pawn = GetPawn<APawn>();
		if (!World || !Pawn)
		{
			return false;
		}

		const FVector TraceEnd = SourceLocation + Direction * DistanceFromSource;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MagicCircleDeployPreview), false, Pawn);
		if (IsValid(MagicCirclePreview))
		{
			QueryParams.AddIgnoredActor(MagicCirclePreview);
		}

		FHitResult HitResult;
		if (!World->LineTraceSingleByChannel(HitResult, SourceLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			return false;
		}

	const FRotator SurfaceRotation = FRotationMatrix::MakeFromZX(HitResult.ImpactNormal, -Direction).Rotator();
		OutTransform = FTransform(SurfaceRotation, HitResult.ImpactPoint);
		return true;
	}

	const FVector DeployLocation = SourceLocation + Direction * DistanceFromSource;
	const FRotator DeployRotation = (-Direction).Rotation();
	OutTransform = FTransform(DeployRotation, DeployLocation);
	return true;
}

void UMagicCircleDeployComponent::SetPreviewTransform(const FTransform& NewTransform, bool bIsValid)
{
	bCanBeDeployed = bIsValid;
	CurrentDeployTransform = bIsValid ? NewTransform : FTransform::Identity;

	if (IsValid(MagicCirclePreview))
	{
		MagicCirclePreview->SetActorHiddenInGame(!bIsValid);
		if (bIsValid)
		{
			MagicCirclePreview->SetActorTransform(NewTransform);
		}
	}
}
