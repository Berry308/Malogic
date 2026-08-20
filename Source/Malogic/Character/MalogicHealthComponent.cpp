// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/MalogicHealthComponent.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/MalogicHealthSet.h"
#include "GameplayEffectExtension.h"
#include "MalogicGameplayTags.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"
#include "System/MalogicAssetManager.h"
#include "System/MalogicGameData.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicHealthComponent)

UMalogicHealthComponent::UMalogicHealthComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DeathState(EMalogicDeathState::NotDead)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UMalogicHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, DeathState);
}

void UMalogicHealthComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	Super::OnUnregister();
}

void UMalogicHealthComponent::InitializeWithAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogMalogic, Error, TEXT("MalogicHealthComponent for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InAbilitySystemComponent;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogMalogic, Error, TEXT("Cannot initialize MalogicHealthComponent for owner [%s] with a null ability system."), *GetNameSafe(Owner));
		return;
	}

	HealthSet = AbilitySystemComponent->GetSet<UMalogicHealthSet>();
	if (!HealthSet)
	{
		UE_LOG(LogMalogic, Warning, TEXT("Cannot initialize MalogicHealthComponent for owner [%s]: the ability system has no health set."), *GetNameSafe(Owner));
		AbilitySystemComponent = nullptr;
		return;
	}

	HealthSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);
	HealthSet->OnOutOfHealth.AddUObject(this, &ThisClass::HandleOutOfHealth);

	AbilitySystemComponent->SetNumericAttributeBase(UMalogicHealthSet::GetHealthAttribute(), HealthSet->GetMaxHealth());
	ClearGameplayTags();

	OnHealthChanged.Broadcast(this, HealthSet->GetHealth(), HealthSet->GetHealth(), nullptr);
	OnMaxHealthChanged.Broadcast(this, HealthSet->GetMaxHealth(), HealthSet->GetMaxHealth(), nullptr);
}

void UMalogicHealthComponent::UninitializeFromAbilitySystem()
{
	ClearGameplayTags();

	if (HealthSet)
	{
		HealthSet->OnHealthChanged.RemoveAll(this);
		HealthSet->OnMaxHealthChanged.RemoveAll(this);
		HealthSet->OnOutOfHealth.RemoveAll(this);
	}

	HealthSet = nullptr;
	AbilitySystemComponent = nullptr;
}

void UMalogicHealthComponent::ClearGameplayTags()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Death_Dying, 0);
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Death_Dead, 0);
	}
}

float UMalogicHealthComponent::GetHealth() const
{
	return HealthSet ? HealthSet->GetHealth() : 0.0f;
}

float UMalogicHealthComponent::GetMaxHealth() const
{
	return HealthSet ? HealthSet->GetMaxHealth() : 0.0f;
}

float UMalogicHealthComponent::GetHealthNormalized() const
{
	const float MaxHealth = GetMaxHealth();
	return MaxHealth > 0.0f ? GetHealth() / MaxHealth : 0.0f;
}

void UMalogicHealthComponent::HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UMalogicHealthComponent::HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
	OnMaxHealthChanged.Broadcast(this, OldValue, NewValue, DamageInstigator);
}

void UMalogicHealthComponent::HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue)
{
#if WITH_SERVER_CODE
	if (AbilitySystemComponent && DamageEffectSpec)
	{
		FGameplayEventData Payload;
		Payload.EventTag = MalogicGameplayTags::GameplayEvent_Death;
		Payload.Instigator = DamageInstigator;
		Payload.Target = AbilitySystemComponent->GetAvatarActor();
		Payload.OptionalObject = DamageEffectSpec->Def;
		Payload.ContextHandle = DamageEffectSpec->GetEffectContext();
		Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
		Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
		Payload.EventMagnitude = DamageMagnitude;

		FScopedPredictionWindow PredictionWindow(AbilitySystemComponent, true);
		AbilitySystemComponent->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
#endif
}

void UMalogicHealthComponent::OnRep_DeathState(EMalogicDeathState OldDeathState)
{
	const EMalogicDeathState NewDeathState = DeathState;
	DeathState = OldDeathState;

	if (OldDeathState > NewDeathState)
	{
		UE_LOG(LogMalogic, Warning, TEXT("Predicted past server death state [%d] -> [%d] for owner [%s]."), static_cast<uint8>(OldDeathState), static_cast<uint8>(NewDeathState), *GetNameSafe(GetOwner()));
		return;
	}

	if (OldDeathState == EMalogicDeathState::NotDead)
	{
		if (NewDeathState == EMalogicDeathState::DeathStarted)
		{
			StartDeath();
		}
		else if (NewDeathState == EMalogicDeathState::DeathFinished)
		{
			StartDeath();
			FinishDeath();
		}
		else
		{
			UE_LOG(LogMalogic, Error, TEXT("Invalid death transition [%d] -> [%d] for owner [%s]."), static_cast<uint8>(OldDeathState), static_cast<uint8>(NewDeathState), *GetNameSafe(GetOwner()));
		}
	}
	else if (OldDeathState == EMalogicDeathState::DeathStarted)
	{
		if (NewDeathState == EMalogicDeathState::DeathFinished)
		{
			FinishDeath();
		}
		else
		{
			UE_LOG(LogMalogic, Error, TEXT("Invalid death transition [%d] -> [%d] for owner [%s]."), static_cast<uint8>(OldDeathState), static_cast<uint8>(NewDeathState), *GetNameSafe(GetOwner()));
		}
	}

	ensureMsgf(DeathState == NewDeathState, TEXT("Death transition failed [%d] -> [%d] for owner [%s]."), static_cast<uint8>(OldDeathState), static_cast<uint8>(NewDeathState), *GetNameSafe(GetOwner()));
}

void UMalogicHealthComponent::StartDeath()
{
	if (DeathState != EMalogicDeathState::NotDead)
	{
		return;
	}

	DeathState = EMalogicDeathState::DeathStarted;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Death_Dying, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);
	OnDeathStarted.Broadcast(Owner);
	Owner->ForceNetUpdate();
}

void UMalogicHealthComponent::FinishDeath()
{
	if (DeathState != EMalogicDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EMalogicDeathState::DeathFinished;
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Death_Dead, 1);
	}

	AActor* Owner = GetOwner();
	check(Owner);
	OnDeathFinished.Broadcast(Owner);
	Owner->ForceNetUpdate();
}

void UMalogicHealthComponent::DamageSelfDestruct(bool bFellOutOfWorld)
{
	if (DeathState != EMalogicDeathState::NotDead || !AbilitySystemComponent)
	{
		return;
	}

	const TSubclassOf<UGameplayEffect> DamageGameplayEffect = UMalogicAssetManager::GetSubclass(UMalogicGameData::Get().DamageGameplayEffect_SetByCaller);
	if (!DamageGameplayEffect)
	{
		UE_LOG(LogMalogic, Error, TEXT("DamageSelfDestruct failed for owner [%s]: unable to find gameplay effect [%s]."), *GetNameSafe(GetOwner()), *UMalogicGameData::Get().DamageGameplayEffect_SetByCaller.GetAssetName());
		return;
	}

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DamageGameplayEffect, 1.0f, AbilitySystemComponent->MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	if (!Spec)
	{
		UE_LOG(LogMalogic, Error, TEXT("DamageSelfDestruct failed for owner [%s]: unable to create a spec for [%s]."), *GetNameSafe(GetOwner()), *GetNameSafe(DamageGameplayEffect));
		return;
	}

	Spec->AddDynamicAssetTag(TAG_Gameplay_DamageSelfDestruct);
	if (bFellOutOfWorld)
	{
		Spec->AddDynamicAssetTag(TAG_Gameplay_FellOutOfWorld);
	}

	Spec->SetSetByCallerMagnitude(MalogicGameplayTags::SetByCaller_Damage, GetMaxHealth());
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec);
}
