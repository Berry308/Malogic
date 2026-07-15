// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "MalogicPawnExtensionComponent.generated.h"

class AActor;
class UMalogicAbilitySystemComponent;
class UMalogicPawnData;

/**
 * Coordinates the Pawn's connection to an ability system owned by another actor.
 *
 * The component deliberately does not depend on Lyra's InitState framework.  Its
 * owning Pawn notifies it when possession, PlayerState replication, or input setup
 * changes, so the ASC can be initialized regardless of replication order.
 */
UCLASS()
class MALOGIC_API UMalogicPawnExtensionComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UMalogicPawnExtensionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Returns the pawn extension component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "Malogic|Pawn")
	static UMalogicPawnExtensionComponent* FindPawnExtensionComponent(const AActor* Actor);

	/** Gets the ASC currently using this pawn as its avatar. */
	UFUNCTION(BlueprintPure, Category = "Malogic|Pawn")
	UMalogicAbilitySystemComponent* GetMalogicAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Gets the PawnData assigned to this pawn. */
	UFUNCTION(BlueprintPure, Category = "Malogic|Pawn")
	const UMalogicPawnData* GetPawnData() const { return PawnData; }

	/** Assigns the PawnData on the server before this pawn consumes it. */
	void SetPawnData(const UMalogicPawnData* InPawnData);

	/** Makes the owning pawn the avatar actor for the supplied ASC. */
	void InitializeAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent, AActor* InOwnerActor);

	/** Removes the owning pawn as the current ASC avatar. */
	void UninitializeAbilitySystem();

	/** Attempts to obtain the PlayerState-owned ASC and initialize it for this pawn. */
	void TryInitializeAbilitySystem();

	/** Notify this component after the pawn's controller changes. */
	void HandleControllerChanged();

	/** Notify this component after PlayerState replication completes. */
	void HandlePlayerStateReplicated();

	/** Notify this component after the pawn input component is created. */
	void SetupPlayerInputComponent();

	/** Registers a callback and invokes it immediately when the ASC is already initialized. */
	void OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

	/** Registers a callback fired when this pawn is removed as the ASC avatar. */
	void OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate Delegate);

	/** Registers a callback and invokes it immediately when PawnData has been assigned. */
	void OnPawnDataInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

	/** Registers a callback and invokes it immediately when the pawn input component exists. */
	void OnPawnInputComponentReady_RegisterAndCall(FSimpleMulticastDelegate::FDelegate Delegate);

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnRep_PawnData();

	void TryInitializePawnData();
	void ApplyPawnDataToAbilitySystem();

private:
	FSimpleMulticastDelegate OnAbilitySystemInitialized;
	FSimpleMulticastDelegate OnAbilitySystemUninitialized;
	FSimpleMulticastDelegate OnPawnDataInitialized;
	FSimpleMulticastDelegate OnPawnInputComponentReady;

	/** Pawn-specific copy of the PlayerState-owned PawnData. */
	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UMalogicPawnData> PawnData;

	/** Cached only while this pawn is the ASC's avatar actor. */
	UPROPERTY(Transient)
	TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent;
};
