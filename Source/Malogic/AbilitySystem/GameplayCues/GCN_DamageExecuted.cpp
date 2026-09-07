// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilitySystem/GameplayCues/GCN_DamageExecuted.h"

#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "MalogicGameplayTags.h"
#include "Message/BattleMessage.h"
#include "Message/MalogicMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GCN_DamageExecuted)

UGCN_DamageExecuted::UGCN_DamageExecuted(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCueTag = MalogicGameplayTags::GameplayCue_UI_Damage;
}

bool UGCN_DamageExecuted::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!IsValid(MyTarget) || Parameters.RawMagnitude <= 0.0f)
	{
		return false;
	}

	if (!Parameters.IsInstigatorLocallyControlledPlayer(MyTarget))
	{
		return false;
	}

	UWorld* World = MyTarget->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UMalogicMessageSubsystem* MessageSubsystem = GameInstance
		? GameInstance->GetSubsystem<UMalogicMessageSubsystem>()
		: nullptr;
	if (!MessageSubsystem)
	{
		return false;
	}

	FGameplayDamageMessage DamageMessage;
	DamageMessage.MessageChannel = MalogicGameplayTags::Message_Combat_Damage;
	DamageMessage.DamageAmount = Parameters.RawMagnitude;
	DamageMessage.WorldLocation = Parameters.Location;
	DamageMessage.DamageType = Parameters.OriginalTag.IsValid()
		? Parameters.OriginalTag
		: Parameters.MatchedTagName;
	DamageMessage.Instigator = Parameters.GetInstigator();
	DamageMessage.EffectCauser = Parameters.GetEffectCauser();
	DamageMessage.Target = MyTarget;

	if (DamageMessage.WorldLocation.IsNearlyZero())
	{
		if (const FHitResult* HitResult = Parameters.EffectContext.GetHitResult(); HitResult && HitResult->bBlockingHit)
		{
			DamageMessage.WorldLocation = HitResult->ImpactPoint;
		}
		else
		{
			DamageMessage.WorldLocation = MyTarget->GetActorLocation();
		}
	}

	MessageSubsystem->BroadcastMessage(DamageMessage.MessageChannel, DamageMessage);
	return true;
}
