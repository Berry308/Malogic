// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "UnLuaInterface.h"
#include "MalogicGameplayAbility_Jump.generated.h"

/**
 * 
 */
UCLASS()
class MALOGIC_API UMalogicGameplayAbility_Jump : public UMalogicGameplayAbility, public IUnLuaInterface
{
	GENERATED_BODY()
	
public:
	UMalogicGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FString GetModuleName_Implementation() const override
	{ return TEXT("Malogic.Abilities.Move.MalogicGA_Jump"); }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveOnAbilityActivated();

	UFUNCTION(BlueprintCallable)
	void CharacterJumpStart();
	UFUNCTION(BlueprintCallable)
	void CharacterJumpStop();
	UFUNCTION(BlueprintCallable)
	void OnInputReleasedLua();
};
