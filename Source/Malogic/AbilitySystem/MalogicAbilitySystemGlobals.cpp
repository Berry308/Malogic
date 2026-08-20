// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/MalogicAbilitySystemGlobals.h"

#include "AbilitySystem/MalogicGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicAbilitySystemGlobals)

UMalogicAbilitySystemGlobals::UMalogicAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UMalogicAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FMalogicGameplayEffectContext();
}
