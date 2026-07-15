// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "System/GameplayTagStack.h"

#include "MalogicPlayerState.generated.h"

class AController;
class AMalogicController;
class APlayerState;
class UAbilitySystemComponent;
class UMalogicAbilitySystemComponent;
class UMalogicPawnData;
struct FGameplayTag;

/**
 * 
 */
UCLASS()
class MALOGIC_API AMalogicPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMalogicPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "MazeRunner|PlayerState")
	AMalogicPlayerController* GetMalogicPlayerController() const;

	UFUNCTION(BlueprintCallable, Category = "MazeRunner|PlayerState")
	UMalogicAbilitySystemComponent* GetMalogicAbilitySystemComponent() const { return AbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }

	/** Sets the persistent PawnData and grants its ability sets on the server. */
	void SetPawnData(const UMalogicPawnData* InPawnData);

	//~AActor interface
	virtual void PreInitializeComponents() override;
	//在构造函数执行后，在BeginPlay()开始前执行
	virtual void PostInitializeComponents() override;
	//~End of AActor interface

	//~APlayerState interface
	virtual void Reset() override;
	virtual void ClientInitialize(AController* C) override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OnDeactivated() override;
	virtual void OnReactivated() override;
	//~End of APlayerState interface

		// Adds a specified number of stacks to the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Removes a specified number of stacks from the tag (does nothing if StackCount is below 1)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// Returns the stack count of the specified tag (or 0 if the tag is not present)
	UFUNCTION(BlueprintCallable, Category = Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// Returns true if there is at least one stack of the specified tag
	UFUNCTION(BlueprintCallable, Category = Teams)
	bool HasStatTag(FGameplayTag Tag) const;

private:
	UFUNCTION()
	void OnRep_PawnData();

	UPROPERTY(ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UMalogicPawnData> PawnData;

	// The ability system component sub-object used by player characters.
	UPROPERTY(VisibleAnywhere, Category = "MazeRunner|PlayerState")
	TObjectPtr<UMalogicAbilitySystemComponent> AbilitySystemComponent;

	//Health attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UMalogicHealthSet> HealthSet;

	//Combat attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UMalogicCombatSet> CombatSet;

	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

};
