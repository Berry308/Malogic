// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Character/MalogicHealthComponent.h"
#include "MVVMViewModelBase.h"

#include "VMPlayerState.generated.h"

/** UI-facing player status maintained by the local player's ViewModel service. */
UCLASS(BlueprintType)
class MALOGIC_API UVMPlayerState : public UMVVMViewModelBase
{
	GENERATED_BODY()

	using ThisClass = UVMPlayerState;

public:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float HealthNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float MagicValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float MaxMagicValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	float MagicNormalized = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Malogic|Player")
	EMalogicDeathState DeathState = EMalogicDeathState::NotDead;

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void Reset();

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetHealth(float InHealth);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetMaxHealth(float InMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetHealthNormalized(float InHealthNormalized);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetMagicValue(float InMagicValue);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetMaxMagicValue(float InMaxMagicValue);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetMagicNormalized(float InMagicNormalized);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetDeathState(EMalogicDeathState InDeathState);
};
