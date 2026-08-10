#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "Magic/MalogicMagicCircleDefinition.h"
#include "MagicCircleDeployComponent.generated.h"

class AActor;
class APawn;
class UMagicCircleManagerComponent;

/** Local-only preview and target preparation for the currently equipped magic circle. */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMagicCircleDeployComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UMagicCircleDeployComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void HandleMagicCirclePreDeploy(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition);

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void UpdatePreDeployMagicCircle(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void IncreaseDeployDistance();

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void DecreaseDeployDistance();

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void SetAndClampDeployDistance(float NewDistance);

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void ClearPreDeployMagicCircle();

	/** Captures the current preview transform for a deployment request. */
	UFUNCTION(BlueprintPure, Category = "Magic Circle Deploy")
	bool GetCurrentDeployTransform(FTransform& OutTransform) const;

	UFUNCTION(BlueprintPure, Category = "Magic Circle Deploy")
	AActor* GetMagicCirclePreview() const { return MagicCirclePreview; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle Deploy")
	bool CanBeDeployed() const { return bCanBeDeployed; }

	UFUNCTION(BlueprintPure, Category = "Magic Circle Deploy")
	float GetDistanceFromSource() const { return DistanceFromSource; }

	UFUNCTION(BlueprintCallable, Category = "Magic Circle Deploy")
	void SetMaxDeployDistanceRatio(float NewRatio);

	UFUNCTION(BlueprintPure, Category = "Magic Circle Deploy")
	float GetMaxDeployDistance() const;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void BindMagicCircleManager();
	void HandleMagicCircleDefinitionChanged(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircleDefinition);

	UFUNCTION()
	void HandlePawnDeathStarted(AActor* OwningActor);

	bool GetDeploymentSource(FVector& OutLocation, FVector& OutDirection) const;
	bool CalculateDeployTransform(const FVector& SourceLocation, const FVector& SourceDirection, FTransform& OutTransform) const;
	void SetPreviewTransform(const FTransform& NewTransform, bool bIsValid);

	UPROPERTY(Transient)
	TObjectPtr<AActor> MagicCirclePreview;

	UPROPERTY(Transient)
	TSubclassOf<UMalogicMagicCircleDefinition> CurrentMagicCircleDefinition;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true"))
	float DistanceFromSource = 0.0f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true"))
	float BaseMaxDeployDistance = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float MaxDeployDistanceRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DeployDistanceStep = 10.0f;

	//Transient当保存关卡（.umap）或资源（.uasset）时，该变量的值不会被保存到文件中。
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true"))
	EMagicCircleDeployStrategy DeployStrategy = EMagicCircleDeployStrategy::CameraRaycast;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Magic Circle Deploy", meta = (AllowPrivateAccess = "true"))
	bool bCanBeDeployed = false;

	UPROPERTY(Transient)
	FTransform CurrentDeployTransform = FTransform::Identity;

	//这个有啥用？
	TWeakObjectPtr<UMagicCircleManagerComponent> MagicCircleManager;
};
