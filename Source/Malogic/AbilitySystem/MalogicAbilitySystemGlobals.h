// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemGlobals.h"

#include "MalogicAbilitySystemGlobals.generated.h"

struct FGameplayEffectContext;

UCLASS(Config = Game)
class MALOGIC_API UMalogicAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	UMalogicAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UAbilitySystemGlobals interface
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
	//~End of UAbilitySystemGlobals interface
};
