#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystem/AbilitySet.h"
#include "GameFramework/Actor.h"
#include "MalogicMagicCircleInstance.generated.h"

class UMalogicAbilitySystemComponent;
class UMalogicCombatSet;
class UMalogicGameplayAbility;
class UMalogicHealthComponent;
class UMalogicHealthSet;
class UMalogicMagicCircleDefinition;
struct FMalogicGATargetData_MagicCircleSpawnInfo;
struct FGameplayTag;
struct FGameplayEffectSpec;

UENUM(BlueprintType)
enum class EMagicCircleState : uint8
{
	Spawned,
	Building,
	Ready,
	Active,
	Finished,
	Destroyed
};

UENUM(BlueprintType)
enum class EMagicCircleLifetimeStrategy : uint8
{
	OnceAfterSomeGA,//一次性魔法阵，在激活某个GA后，魔法阵就会结束
	PersistentTilDie
};

UENUM(BlueprintType)
enum class EMagicCircleActivateStrategy : uint8
{
	Auto,
	Manual,
	Detection
};

UCLASS(BlueprintType, Blueprintable)
class MALOGIC_API AMalogicMagicCircleInstance : public AActor
{
	GENERATED_BODY()

public:
	AMalogicMagicCircleInstance();

	virtual void InitializeFromDefinition(const UMalogicMagicCircleDefinition* Definition, AActor* InInstigator, float InActualBuildingTime);
	virtual void InitializeFromTargetData(FMalogicGATargetData_MagicCircleSpawnInfo& SpawnInfo);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Magic Circle")
	void ActivateMagic(const FGameplayTag& ActivationTag);

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	EMagicCircleState GetMagicCircleState() const { return MagicCircleState; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	AActor* GetDeploymentInstigator() const { return DeploymentInstigator; }

	UMalogicAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeLifetime();
	void StartBuilding();

	UFUNCTION()
	virtual void HandleBuildingFinished();
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle")
	void K2_OnBuildingFinished();

	virtual void HandleMagicCircleReady();
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle")
	void K2_OnMagicCircleReady();

	virtual void HandleMagicCircleFinished();
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle")
	void K2_OnMagicCircleFinished();

	virtual void HandleMagicCircleDestroyed();
	UFUNCTION(BlueprintImplementableEvent, Category = "Magic Circle")
	void K2_OnMagicCircleDestroyed();

	UFUNCTION()
	void OnRep_MagicCircleState(EMagicCircleState OldState);

	void OnMagicCircleStateChanged(EMagicCircleState OldState, EMagicCircleState NewState);

	void SetMagicCircleState(EMagicCircleState NewState);
	bool ActivateAbilitiesByTag(const FGameplayTag& ActivationTag);
	virtual void HandleActivateMagicFail(const FGameplayTag& ActivationTag);
	void StartLifeTimeTimer();
	void OnMagicCircleLifeTimeEnded();
	void OnAbilityFinished(const FAbilityEndedData& AbilityEndedData);
	void FinishMagicCircle();
	void HandleOutOfHealth();
	void HandleOutOfHealthEvent(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	EMagicCircleLifetimeStrategy LifetimeStrategy = EMagicCircleLifetimeStrategy::OnceAfterSomeGA;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	EMagicCircleActivateStrategy ActivateStrategy = EMagicCircleActivateStrategy::Auto;

	UPROPERTY(Replicated)
	float ActualBuildingTime = 0.0f;

	//提供给魔法阵进行动画播放速度计算。
	UPROPERTY(BlueprintReadOnly)
	float BaseBuildingTime = 0.0f;

	//如果OnRep函数中声明了一个与同步属性类型相同的参数，引擎会自动将同步发生前的本地值（Old Value）作为参数传入。
	UPROPERTY(ReplicatedUsing = OnRep_MagicCircleState)
	EMagicCircleState MagicCircleState = EMagicCircleState::Spawned;

	UPROPERTY(Replicated)
	TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinitionClass;

	UPROPERTY()
	TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UMalogicHealthComponent> HealthComponent;

	//AttributeSet通过AbilitySet添加到ASC中，
	UPROPERTY()
	TObjectPtr<const UMalogicHealthSet> HealthSet;

	UPROPERTY()
	TObjectPtr<const UMalogicCombatSet> CombatSet;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> DeploymentInstigator;

	FAbilitySet_GrantedHandles GrantedHandles;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LifeTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Magic Circle", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UMalogicGameplayAbility> FinishAbilityClass;

	FTimerHandle BuildingTimerHandle;
	FTimerHandle LifeTimeTimerHandle;
	float ElapsedTime = 0.0f;
	bool bLifeTimeExpired = false;
	bool bAbilitySetGranted = false;
};
