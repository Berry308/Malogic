// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/VMPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VMPlayerState)

void UVMPlayerState::Reset()
{
	SetHealth(0.0f);
	SetMaxHealth(0.0f);
	SetHealthNormalized(0.0f);
	SetMagicValue(0.0f);
	SetMaxMagicValue(0.0f);
	SetMagicNormalized(0.0f);
	SetDeathState(EMalogicDeathState::NotDead);
}

void UVMPlayerState::SetHealth(float InHealth)
{
	UE_MVVM_SET_PROPERTY_VALUE(Health, InHealth);
}

void UVMPlayerState::SetMaxHealth(float InMaxHealth)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, InMaxHealth);
}

void UVMPlayerState::SetHealthNormalized(float InHealthNormalized)
{
	UE_MVVM_SET_PROPERTY_VALUE(HealthNormalized, InHealthNormalized);
}

void UVMPlayerState::SetMagicValue(float InMagicValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MagicValue, InMagicValue);
}

void UVMPlayerState::SetMaxMagicValue(float InMaxMagicValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxMagicValue, InMaxMagicValue);
}

void UVMPlayerState::SetMagicNormalized(float InMagicNormalized)
{
	UE_MVVM_SET_PROPERTY_VALUE(MagicNormalized, InMagicNormalized);
}

void UVMPlayerState::SetDeathState(EMalogicDeathState InDeathState)
{
	UE_MVVM_SET_PROPERTY_VALUE(DeathState, InDeathState);
}
