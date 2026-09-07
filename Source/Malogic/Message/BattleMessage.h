// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "BattleMessage.generated.h"

class AActor;

/**
 * Data broadcast when a gameplay effect has dealt damage that can be shown to the local player.
 *
 * The message is transient and is not replicated by UMalogicMessageSubsystem. Actor references
 * are weak because the payload may be copied or queued by a presentation system after broadcast.
 */
USTRUCT(BlueprintType)
struct MALOGIC_API FGameplayDamageMessage
{
	GENERATED_BODY()

	/** Message channel used by the local MessageSubsystem broadcast. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Message")
	FGameplayTag MessageChannel;

	/** Final damage amount after gameplay calculation. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	float DamageAmount = 0.0f;

	/** World-space position where the damage was applied, normally the hit impact point. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	FVector WorldLocation = FVector::ZeroVector;

	/** Gameplay Tag identifying the damage or elemental type. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	FGameplayTag DamageType;

	/** Gameplay Tag identifying an elemental reaction, if one occurred. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	FGameplayTag ReactionTag;

	/** Original source actor that instigated the damage. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	TWeakObjectPtr<AActor> Instigator;

	/** Actor that physically caused the gameplay effect, such as a projectile. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	TWeakObjectPtr<AActor> EffectCauser;

	/** Actor that received the damage. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay|Damage")
	TWeakObjectPtr<AActor> Target;
};
