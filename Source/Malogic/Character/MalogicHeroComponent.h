// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "MalogicHeroComponent.generated.h"

namespace EEndPlayReason { enum Type : int; }

class UMalogicInputConfig;
class UObject;
class UInputComponent;
class UInputMappingContext;
struct FGameplayTag;
struct FInputActionValue;

USTRUCT()
struct FInputMappingContextAndPriority
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input", meta = (AssetBundles = "Client,Server"))
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	// Higher priority input mappings will be prioritized over mappings with a lower priority.
	UPROPERTY(EditAnywhere, Category = "Input")
	int32 Priority = 0;

	/** If true, then this mapping context will be registered with the settings when this game feature action is registered. */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bRegisterWithSettings = true;
};

/**
 * 
 */
UCLASS()
class MALOGIC_API UMalogicHeroComponent : public UPawnComponent
{
	GENERATED_BODY()
public:
	UMalogicHeroComponent(const FObjectInitializer& ObjectInitializer);

	/** Returns the hero component if one exists on the specified actor. */
	UFUNCTION(BlueprintPure, Category = "Malogic|Hero")
	static UMalogicHeroComponent* FindHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UMalogicHeroComponent>() : nullptr); }

	/** Adds mode-specific input config 被GFA_InputBinding所调用*/
	void AddAdditionalInputConfig(const UMalogicInputConfig* InputConfig);

	/** Removes a mode-specific input config if it has been added */
	void RemoveAdditionalInputConfig(const UMalogicInputConfig* InputConfig);

	/** True if this is controlled by a real player and has progressed far enough in initialization where additional input bindings can be added */
	bool IsReadyToBindInputs() const;

	/** The name of the extension event sent via UGameFrameworkComponentManager when ability inputs are ready to bind */
	static const FName NAME_BindInputsNow;

	/** The name of this component-implemented feature */
	static const FName NAME_ActorFeatureName;

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Initializes the local hero after PawnData, ASC, and input are all ready. */
	void TryInitializeHero();

	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);

	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	void Input_LookStick(const FInputActionValue& InputActionValue);
	void Input_Crouch(const FInputActionValue& InputActionValue);


protected:
	UPROPERTY(EditAnywhere)
	UMalogicInputConfig* DefaultInputConfig;

	UPROPERTY(EditAnywhere)
	TArray<FInputMappingContextAndPriority> DefaultInputMappings;//From GameFeatureAction_AddInputContextMapping.h

	bool bReadyToBindInputs;

};
