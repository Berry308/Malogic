#pragma once

#include "AbilitySystem/AbilitySet.h"
#include "Character/MalogicHeroComponent.h"
#include "MalogicMagicCircleDefinition.h"
#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "MagicCircleManagerComponent.generated.h"

class AController;
class APawn;
class UMalogicAbilitySystemComponent;
class UEnhancedInputLocalPlayerSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FMagicCircleDefinitionChanged, TSubclassOf<UMalogicMagicCircleDefinition>);

/** Owns the magic circle definition currently equipped by a pawn. */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class MALOGIC_API UMagicCircleManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UMagicCircleManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Magic Circle")
	bool EquipMagicCircle(TSubclassOf<UMalogicMagicCircleDefinition> NewMagicCircle);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Magic Circle")
	void UnequipMagicCircle();

	UFUNCTION(BlueprintPure, Category = "Magic Circle")
	TSubclassOf<UMalogicMagicCircleDefinition> GetEquippedMagicCircle() const { return EquippedMagicCircle; }

	/** Notifies local systems after the equipped definition has been synchronized. */
	FMagicCircleDefinitionChanged OnMagicCircleDefinitionChanged;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ApplyAbilitySetOnServer(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircle);
	void RemoveAbilitySetOnServer();

	void ApplyInputMappingForLocalPlayer(TSubclassOf<UMalogicMagicCircleDefinition> MagicCircle);
	void RemoveInputMappingForLocalPlayer();

	UFUNCTION()
	void OnRep_EquippedMagicCircle(TSubclassOf<UMalogicMagicCircleDefinition> PreviousMagicCircle);

private:
	void BindPawnExtensionDelegates();
	void HandleAbilitySystemInitialized();
	void HandleAbilitySystemUninitialized();
	UFUNCTION()
	void HandlePawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UMalogicAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	void CancelActiveAbilitiesFromDefinition(const UMalogicMagicCircleDefinition* Definition);

	void RequestLocalStateSync();
	void CompleteLocalStateSync();
	bool TrySynchronizeLocalState();
	bool IsLocalInputReady() const;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedMagicCircle)
	TSubclassOf<UMalogicMagicCircleDefinition> EquippedMagicCircle;

	// Only the authoritative side uses these handles to remove this definition's grants.
	FAbilitySet_GrantedHandles EquippedAbilitySetHandles;

	// Only the locally controlled pawn uses this mapping to remove the exact context it added.
	FInputMappingContextAndPriority AppliedInputMapping;
	TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> AppliedInputSubsystem;
	bool bHasAppliedInputMapping = false;

	bool bAbilitySetApplied = false;
	bool bPawnExtensionDelegatesBound = false;
	bool bLocalStateSyncPending = false;
	bool bDefinitionNotificationPending = false;
};
