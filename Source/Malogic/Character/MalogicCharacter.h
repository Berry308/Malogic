// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"
#include "ModularCharacter.h"

#include "MalogicCharacter.generated.h"

class AController;
class AActor;
class AMalogicPlayerController;
class AMalogicPlayerState;
class UAbilitySystemComponent;
class UCameraComponent;
class UDamageType;
class UInputComponent;
class UMalogicAbilitySystemComponent;
class UMalogicCharacterMovementComp;
class UMalogicHealthComponent;
class UMalogicPawnExtensionComponent;
class USkeletalMeshComponent;
struct FGameplayTag;
struct FGameplayTagContainer;

/** Base character pawn responsible for forwarding lifecycle events to gameplay components. */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base character pawn class used by this project."))
class MALOGIC_API AMalogicCharacter : public AModularCharacter, public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AMalogicCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Malogic|Character")
	AMalogicPlayerController* GetMalogicPlayerController() const;

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	UFUNCTION(BlueprintCallable, Category = "Malogic|Character")
	AMalogicPlayerState* GetMalogicPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|Character")
	UMalogicAbilitySystemComponent* GetMalogicAbilitySystemComponent() const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void ToggleCrouch();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Malogic|Character")
	FString CharacterFormalName;

protected:
	virtual void Reset() override;
	virtual void NotifyControllerChanged() override;

	virtual void OnAbilitySystemInitialized();
	virtual void OnAbilitySystemUninitialized();
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void InitializeGameplayTags();
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	UFUNCTION()
	virtual void OnDeathStarted(AActor* OwningActor);

	UFUNCTION()
	virtual void OnDeathFinished(AActor* OwningActor);

	void DisableMovementAndCollision();
	void DestroyDueToDeath();
	void UninitAndDestroy();

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDeathFinished"))
	void K2_OnDeathFinished();

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual bool CanJumpInternal_Implementation() const override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malogic|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMalogicPawnExtensionComponent> PawnExtensionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malogic|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMalogicHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malogic|Character", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;
};
