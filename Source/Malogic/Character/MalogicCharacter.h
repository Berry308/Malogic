// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"

#include "MalogicCharacter.generated.h"

class AMalogicPlayerController;
class AMalogicPlayerState;
class UCameraComponent;
class UMRHealthComponent;
class UMalogicAbilitySystemComponent;
class UObject;

struct FFrame;
struct FGameplayTag;
struct FGameplayTagContainer;

/**
 * 
 */
UCLASS()
class MALOGIC_API AMalogicCharacter : public AModularCharacter/*, public IAbilitySystemInterface, public IGameplayCueInterface, public IGameplayTagAssetInterface*/
{
	GENERATED_BODY()
	
	AMalogicCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

//	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
//	AMalogicPlayerController* GetMalogicPlayerController() const;
//
//	/** Returns the first person mesh **/
//	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
//	/** Returns first person camera component **/
//	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
//
//	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
//	AMalogicPlayerState* GetMalogicPlayerState() const;
//
//	UFUNCTION(BlueprintCallable, Category = "Lyra|Character")
//	UMalogicAbilitySystemComponent* GetMalogicAbilitySystemComponent() const;
//	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
//
//	//~IGameplayTagAssetInterface
//	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
//	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;
//	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
//	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
//	//~End of IGameplayTagAssetInterface
//
//	void ToggleCrouch();
//
//	//~AActor interface
//	virtual void PreInitializeComponents() override;
//	virtual void BeginPlay() override;
//	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//	virtual void Reset() override;
//	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
//	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;
//	//~End of AActor interface
//
//	//~APawn interface
//	virtual void NotifyControllerChanged() override;
//	//~End of APawn interface
//
//protected:
//	//被绑定到PawnExtensionComponent上去了，因为是PawnExtensionComponent触发初始化ASC的
//	virtual void OnAbilitySystemInitialized();
//	virtual void OnAbilitySystemUninitialized();
//
//	virtual void PossessedBy(AController* NewController) override;
//	virtual void UnPossessed() override;
//
//	virtual void OnRep_Controller() override;
//	virtual void OnRep_PlayerState() override;
//
//	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
//
//	//被AMRCharacter::OnAbilitySystemInitialized调用
//	void InitializeGameplayTags();
//
//	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
//
//	// Begins the death sequence for the character (disables collision, disables movement, etc...)
//	UFUNCTION()
//	virtual void OnDeathStarted(AActor* OwningActor);
//
//	// Ends the death sequence for the character (detaches controller, destroys pawn, etc...)
//	UFUNCTION()
//	virtual void OnDeathFinished(AActor* OwningActor);
//
//	void DisableMovementAndCollision();
//	void DestroyDueToDeath();
//	void UninitAndDestroy();
//
//	// Called when the death sequence for the character has completed
//	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "OnDeathFinished"))
//	void K2_OnDeathFinished();
//
//	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) override;
//	void SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled);
//
//	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
//	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
//
//	//用于内部判断是否能跳跃
//	virtual bool CanJumpInternal_Implementation() const;
//
//public:
//	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "MazeRunner|Character")
//	FString CharacterFormalName;
//
//private:
//
//
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lyra|Character", Meta = (AllowPrivateAccess = "true"))
//	TObjectPtr<UMRHealthComponent> HealthComponent;
//
//	/** Pawn mesh: first person view (arms; seen only by self) */
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
//	USkeletalMeshComponent* FirstPersonMesh;
//
//	// 第一人称相机组件
//	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MazeRunner|Character", Meta = (AllowPrivateAccess = "true"))
//	TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

};
