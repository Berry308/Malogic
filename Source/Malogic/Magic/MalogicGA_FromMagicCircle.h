// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/MalogicGameplayAbility.h"
#include "MalogicGA_FromMagicCircle.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class MALOGIC_API UMalogicGA_FromMagicCircle : public UMalogicGameplayAbility
{
	GENERATED_BODY()

public:
	UMalogicGA_FromMagicCircle(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
