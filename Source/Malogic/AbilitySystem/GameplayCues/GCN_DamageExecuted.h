// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"

#include "GCN_DamageExecuted.generated.h"

/** Converts an executed damage GameplayCue into a local damage message for UI presentation. */
UCLASS(Blueprintable)
class MALOGIC_API UGCN_DamageExecuted : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UGCN_DamageExecuted(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
