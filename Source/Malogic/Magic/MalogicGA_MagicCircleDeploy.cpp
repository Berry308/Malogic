#include "Magic/MalogicGA_MagicCircleDeploy.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "AbilitySystem/TargetData/MalogicGATargetData_MagicCircleSpawnInfo.h"
#include "Engine/World.h"
#include "Equipment/MalogicEquipmentManagerComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Magic/MagicCircleDeployComponent.h"
#include "Magic/MagicCircleViewActor.h"
#include "Magic/MalogicMagicCircleDefinition.h"
#include "Magic/MalogicMagicCircleInstance.h"
#include "MalogicLogChannels.h"
#include "Weapon/MagicWeaponStateComponent.h"
#include "Weapon/MalogicMagicWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGA_MagicCircleDeploy)

UE_DEFINE_GAMEPLAY_TAG_STATIC(Ability_MagicWeapon_NoFiring, "Ability.MagicWeapon.NoFiring");

UMalogicGA_MagicCircleDeploy::UMalogicGA_MagicCircleDeploy(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationBlockedTags.AddTag(Ability_MagicWeapon_NoFiring);
}

bool UMalogicGA_MagicCircleDeploy::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UMalogicMagicCircleDefinition* Definition = GetAssociatedDefinition(Handle, ActorInfo);
	if (!Definition || !Definition->MagicCircleToSpawn)
	{
		return false;
	}

	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		const APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
		const UMagicCircleDeployComponent* DeployComponent = AvatarPawn ? AvatarPawn->FindComponentByClass<UMagicCircleDeployComponent>() : nullptr;
		return DeployComponent && DeployComponent->CanBeDeployed();
	}

	return true;
}

void UMalogicGA_MagicCircleDeploy::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	OnTargetDataReadyCallbackDelegateHandle = AbilitySystemComponent->AbilityTargetDataSetDelegate(
		CurrentSpecHandle,
		CurrentActivationInfo.GetActivationPredictionKey()).AddUObject(this, &ThisClass::OnTargetDataReadyCallback);

	if (UMalogicMagicWeaponInstance* MagicWeapon = GetMagicWeaponInstance(ActorInfo))
	{
		MagicWeapon->UpdateFiringTime();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ActorInfo && ActorInfo->IsLocallyControlled())
	{
		StartDeploymentTargeting();
	}
}

void UMalogicGA_MagicCircleDeploy::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo))
	{
		return;
	}

	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}

	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponentFromActorInfo())
	{
		AbilitySystemComponent->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(OnTargetDataReadyCallbackDelegateHandle);
		AbilitySystemComponent->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

//使用DeployComponent得到魔法阵的部署位置和朝向，可以被子类重写
bool UMalogicGA_MagicCircleDeploy::CalculateDeployTransform(const FGameplayAbilityActorInfo* ActorInfo, FTransform& OutDeployTransform) const
{
	const APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	const UMagicCircleDeployComponent* DeployComponent = AvatarPawn ? AvatarPawn->FindComponentByClass<UMagicCircleDeployComponent>() : nullptr;
	return DeployComponent && DeployComponent->CanBeDeployed() && DeployComponent->GetCurrentDeployTransform(OutDeployTransform);
}

float UMalogicGA_MagicCircleDeploy::CalculateActualBuildingTime(const UMalogicMagicCircleDefinition* Definition, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const float BaseBuildingTime = Definition ? FMath::Max(0.0f, Definition->BaseBuildingTime) : MinimumActualBuildingTime;

	//从MagicWeapon获取魔法阵的构建速度加成
	const UMalogicMagicWeaponInstance* MagicWeapon = GetMagicWeaponInstance(ActorInfo);
	const float BuildingRate = MagicWeapon ? FMath::Max(KINDA_SMALL_NUMBER, MagicWeapon->GetMagicCircleBuildingRate()) : 1.0f;

	return FMath::Clamp(BaseBuildingTime / BuildingRate, MinimumActualBuildingTime, MaximumActualBuildingTime);
}

const UMalogicMagicCircleDefinition* UMalogicGA_MagicCircleDeploy::GetAssociatedDefinition(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Cast<UMalogicMagicCircleDefinition>(GetSourceObject(Handle, ActorInfo));
}

UMalogicMagicWeaponInstance* UMalogicGA_MagicCircleDeploy::GetMagicWeaponInstance(const FGameplayAbilityActorInfo* ActorInfo) const
{
	APawn* AvatarPawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	UMalogicEquipmentManagerComponent* EquipmentManager = AvatarPawn ? AvatarPawn->FindComponentByClass<UMalogicEquipmentManagerComponent>() : nullptr;
	UMalogicMagicWeaponInstance* MagicWeapon = EquipmentManager ? EquipmentManager->GetFirstInstanceOfType<UMalogicMagicWeaponInstance>() : nullptr;
	if(!MagicWeapon)
	{
		UE_LOG(LogMalogic, Warning, TEXT("Failed to get magic weapon instance for ability [%s]."), *GetNameSafe(this));
	}
	return MagicWeapon;
}

bool UMalogicGA_MagicCircleDeploy::ValidateDeploymentTargetData(const FGameplayAbilityTargetDataHandle& TargetData, FTransform& OutDeployTransform) const
{
	// TargetData.Num() != 1 是因为一次部署只允许一个目标变换。
	if (TargetData.Num() != 1)
	{
		return false;
	}

	const FGameplayAbilityTargetData* RawTargetData = TargetData.Get(0);
	if (!RawTargetData || !RawTargetData->GetScriptStruct()->IsChildOf(FMalogicGATargetData_MagicCircleSpawnInfo::StaticStruct()))
	{
		return false;
	}

	OutDeployTransform = RawTargetData->GetEndPointTransform();
	if (OutDeployTransform.ContainsNaN() || !OutDeployTransform.GetRotation().IsNormalized())
	{
		return false;
	}

	// The source transform is client-provided target data and must not be used for
	// authorization. Validate the submitted endpoint against the server pawn.
	const APawn* AvatarPawn = CurrentActorInfo ? Cast<APawn>(CurrentActorInfo->AvatarActor.Get()) : nullptr;
	const UMalogicMagicCircleDefinition* Definition = GetAssociatedDefinition(CurrentSpecHandle, CurrentActorInfo);
	if (!AvatarPawn || !Definition || !FMath::IsFinite(Definition->BaseMaxDeployDistance) || Definition->BaseMaxDeployDistance < 0.0f)
	{
		return false;
	}

	const FVector PawnLocation = AvatarPawn->GetActorLocation();
	const FVector DeployLocation = OutDeployTransform.GetLocation();
	if (PawnLocation.ContainsNaN() || DeployLocation.ContainsNaN())
	{
		return false;
	}

	//最大部署距离的获取方式在未来可能需要修改，使用一个特定的函数接口以便获取在经过buff或其他因素影响后的最大部署距离。
	const float MaxDeployDistance = Definition->BaseMaxDeployDistance;
	const float DistanceSquared = FVector::DistSquared(PawnLocation, DeployLocation);
	if (DistanceSquared > FMath::Square(MaxDeployDistance))
	{
		UE_LOG(LogMalogic, Warning,
			TEXT("Magic circle deployment rejected for [%s]: endpoint distance [%f] exceeds maximum [%f]."),
			*GetNameSafe(AvatarPawn), FMath::Sqrt(DistanceSquared), MaxDeployDistance);
		return false;
	}

	return true;
}

AMalogicMagicCircleInstance* UMalogicGA_MagicCircleDeploy::SpawnMagicCircleInstance(const UMalogicMagicCircleDefinition* Definition, const FGameplayAbilityActorInfo* ActorInfo, const FTransform& DeployTransform, float ActualBuildingTime, FMalogicGATargetData_MagicCircleSpawnInfo& SpawnInfo) const
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Definition || !Definition->MagicCircleToSpawn)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	APawn* AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!World || !AvatarPawn)
	{
		return nullptr;
	}

	AMalogicMagicCircleInstance* MagicCircleInstance = World->SpawnActorDeferred<AMalogicMagicCircleInstance>(
		Definition->MagicCircleToSpawn,DeployTransform,
		AvatarPawn,AvatarPawn,
		ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding
	);

	if (!MagicCircleInstance)
	{
		UE_LOG(LogMalogic, Error, TEXT("Magic circle deployment failed to spawn instance [%s]."), *GetNameSafe(Definition->MagicCircleToSpawn));
		return nullptr;
	}

	MagicCircleInstance->InitializeFromDefinition(Definition, AvatarPawn, ActualBuildingTime);
	MagicCircleInstance->InitializeFromTargetData(SpawnInfo);
	MagicCircleInstance->FinishSpawning(DeployTransform);
	return IsValid(MagicCircleInstance) ? MagicCircleInstance : nullptr;
}

void UMalogicGA_MagicCircleDeploy::StartDeploymentTargeting()
{
	check(CurrentActorInfo)
	if (!CurrentActorInfo->IsLocallyControlled())
	{
		UE_LOG(LogMalogic, Warning, TEXT("StartDeploymentTargeting called on non-local controller for ability [%s]."), *GetNameSafe(this));
		return;
	}

	UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponentFromActorInfo();
	const UMalogicMagicCircleDefinition* Definition = GetAssociatedDefinition(CurrentSpecHandle, CurrentActorInfo);
	if (!AbilitySystemComponent || !Definition)
	{
		UE_LOG(LogMalogic, Error, TEXT("StartDeploymentTargeting failed to get ability system component or definition for ability [%s]."), *GetNameSafe(this));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	FTransform DeployTransform;
	if (!CalculateDeployTransform(CurrentActorInfo, DeployTransform))
	{
		UE_LOG(LogMalogic, Warning, TEXT("StartDeploymentTargeting failed to calculate deploy transform for ability [%s]."), *GetNameSafe(this));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	AController* Controller = GetControllerFromActorInfo();
	check(Controller);

	UMagicWeaponStateComponent* WeaponStateComponent = Controller ? Controller->FindComponentByClass<UMagicWeaponStateComponent>() : nullptr;
	check(WeaponStateComponent);

	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent, CurrentActivationInfo.GetActivationPredictionKey());

	FGameplayAbilityTargetDataHandle TargetData;
	TargetData.UniqueId = WeaponStateComponent->AllocatePredictiveViewId();

	FMalogicGATargetData_MagicCircleSpawnInfo* SpawnInfo = new FMalogicGATargetData_MagicCircleSpawnInfo();
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			SpawnInfo->ClientSpawnTime = GameState->GetServerWorldTimeSeconds();
		}
	}
	SpawnInfo->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	SpawnInfo->SourceLocation.LiteralTransform = CurrentActorInfo->AvatarActor.IsValid() ? CurrentActorInfo->AvatarActor->GetActorTransform() : FTransform::Identity;
	SpawnInfo->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	SpawnInfo->TargetLocation.LiteralTransform = DeployTransform;
	TargetData.Add(SpawnInfo);

	if (WeaponStateComponent && Definition->ViewActorForPrediction)
	{
		WeaponStateComponent->AddUnconfirmedPredictiveViewActor(TargetData, Definition->ViewActorForPrediction, CalculateActualBuildingTime(Definition, CurrentActorInfo));
	}

	OnTargetDataReadyCallback(TargetData, FGameplayTag());
}

void UMalogicGA_MagicCircleDeploy::OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag)
{
	UAbilitySystemComponent* AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
	check(AbilitySystemComponent);

	if (const FGameplayAbilitySpec* AbilitySpec = AbilitySystemComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
	{
		FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent);
		FGameplayAbilityTargetDataHandle LocalTargetDataHandle(MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

		if (CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority())
		{
			AbilitySystemComponent->CallServerSetReplicatedTargetData(
				CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey(),
				LocalTargetDataHandle,ApplicationTag,AbilitySystemComponent->ScopedPredictionKey);
		}

		bool bIsTargetDataValid = true;

		bool bDeploymentSucceeded = true;

#if WITH_SERVER_CODE
		if (AController* Controller = GetControllerFromActorInfo())
		{
			if (Controller->GetLocalRole() == ROLE_Authority)
			{
				FTransform DeployTransform;
				const UMalogicMagicCircleDefinition* Definition = GetAssociatedDefinition(CurrentSpecHandle, CurrentActorInfo);
				bIsTargetDataValid = ValidateDeploymentTargetData(LocalTargetDataHandle, DeployTransform);
				FMalogicGATargetData_MagicCircleSpawnInfo* SpawnInfo = bIsTargetDataValid
					? static_cast<FMalogicGATargetData_MagicCircleSpawnInfo*>(LocalTargetDataHandle.Get(0))
					: nullptr;
				bIsTargetDataValid = bIsTargetDataValid && SpawnInfo != nullptr;

				//这个地方有股异味，CommitAbility和SpawnMagicCircleInstance的调用顺序可能会影响游戏逻辑，应该仔细考虑是否需要调整。
				//如果CommitAbility在前面，那么SpawnMagicCircleInstance可能会失败，这就导致了无用的消耗
				//如果SpawnMagicCircleInstance在前面，那么CommitAbility可能会失败，这就导致了魔法阵被销毁，尽管结果正确，但是可能会有一些不必要的开销（在魔力不够的时候）
				AMalogicMagicCircleInstance* SpawnedMagicCircle = nullptr;
				if (bIsTargetDataValid && SpawnInfo)
				{
					SpawnedMagicCircle = SpawnMagicCircleInstance(Definition, CurrentActorInfo, DeployTransform, CalculateActualBuildingTime(Definition, CurrentActorInfo), *SpawnInfo);
				}
				
				bDeploymentSucceeded = IsValid(SpawnedMagicCircle)
					&& CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
				if (!bDeploymentSucceeded && SpawnedMagicCircle)
				{
					SpawnedMagicCircle->Destroy();
				}

				if (UMagicWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<UMagicWeaponStateComponent>())
				{
					//Question：这里能够保证网络同步Actor和RPC同时或同一批次到达吗？
					WeaponStateComponent->ClientConfirmTargetData(LocalTargetDataHandle.UniqueId, bDeploymentSucceeded);
				}
			}
		}
#endif

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bDeploymentSucceeded);
	}
}

