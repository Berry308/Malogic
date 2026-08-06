#include "AbilitySystem/Attributes/MalogicMagicSet.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "MalogicLogChannels.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicMagicSet)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MagicUnlimited, "Gameplay.Magic.Unlimited");

UMalogicMagicSet::UMalogicMagicSet()
	: MagicValue(100.0f)
	, MaxMagicValue(100.0f)
	, bOutOfMagic(false)
	, MagicValueBeforeAttributeChange(0.0f)
	, MaxMagicValueBeforeAttributeChange(0.0f)
	, MagicConsume(0.0f)
	, MagicRecovery(0.0f)
{
}

void UMalogicMagicSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMalogicMagicSet, MagicValue, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMalogicMagicSet, MaxMagicValue, COND_None, REPNOTIFY_Always);
}

void UMalogicMagicSet::OnRep_MagicValue(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMalogicMagicSet, MagicValue, OldValue);

	const float OldMagicValue = OldValue.GetCurrentValue();
	const float CurrentMagicValue = GetMagicValue();
	const float EstimatedMagnitude = CurrentMagicValue - OldMagicValue;

	OnMagicValueChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldMagicValue, CurrentMagicValue);

	if (!bOutOfMagic && CurrentMagicValue <= 0.0f)
	{
		OnOutOfMagic.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldMagicValue, CurrentMagicValue);
	}

	bOutOfMagic = CurrentMagicValue <= 0.0f;
}

void UMalogicMagicSet::OnRep_MaxMagicValue(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMalogicMagicSet, MaxMagicValue, OldValue);

	const float OldMaxMagicValue = OldValue.GetCurrentValue();
	OnMaxMagicValueChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxMagicValue() - OldMaxMagicValue, OldMaxMagicValue, GetMaxMagicValue());
}

bool UMalogicMagicSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if (!Super::PreGameplayEffectExecute(Data))
	{
		return false;
	}

	if (Data.EvaluatedData.Attribute == GetMagicConsumeAttribute()
		&& Data.EvaluatedData.Magnitude > 0.0f
		&& Data.Target.HasMatchingGameplayTag(TAG_Gameplay_MagicUnlimited))
	{
		Data.EvaluatedData.Magnitude = 0.0f;
		return false;
	}

	MagicValueBeforeAttributeChange = GetMagicValue();
	MaxMagicValueBeforeAttributeChange = GetMaxMagicValue();

	return true;
}

void UMalogicMagicSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetEffectContext();
	AActor* Instigator = EffectContext.GetOriginalInstigator();
	AActor* Causer = EffectContext.GetEffectCauser();

	if (Data.EvaluatedData.Attribute == GetMagicConsumeAttribute())
	{
		UE_LOG(LogMRAbilitySystem, Verbose, TEXT("%s consumed %f magic."), *GetNameSafe(this), GetMagicConsume());
		SetMagicValue(FMath::Clamp(GetMagicValue() - GetMagicConsume(), 0.0f, GetMaxMagicValue()));
		SetMagicConsume(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetMagicRecoveryAttribute())
	{
		UE_LOG(LogMRAbilitySystem, Verbose, TEXT("%s recovered %f magic."), *GetNameSafe(this), GetMagicRecovery());
		SetMagicValue(FMath::Clamp(GetMagicValue() + GetMagicRecovery(), 0.0f, GetMaxMagicValue()));
		SetMagicRecovery(0.0f);
	}
	else if (Data.EvaluatedData.Attribute == GetMagicValueAttribute())
	{
		SetMagicValue(FMath::Clamp(GetMagicValue(), 0.0f, GetMaxMagicValue()));
	}
	else if (Data.EvaluatedData.Attribute == GetMaxMagicValueAttribute())
	{
		OnMaxMagicValueChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxMagicValueBeforeAttributeChange, GetMaxMagicValue());
	}

	if (GetMagicValue() != MagicValueBeforeAttributeChange)
	{
		OnMagicValueChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MagicValueBeforeAttributeChange, GetMagicValue());
	}

	if (GetMagicValue() <= 0.0f && !bOutOfMagic)
	{
		OnOutOfMagic.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MagicValueBeforeAttributeChange, GetMagicValue());
	}

	bOutOfMagic = GetMagicValue() <= 0.0f;
}

void UMalogicMagicSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMalogicMagicSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	ClampAttribute(Attribute, NewValue);
}

void UMalogicMagicSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxMagicValueAttribute() && GetMagicValue() > NewValue)
	{
		UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent();
		check(AbilitySystemComponent);

		AbilitySystemComponent->ApplyModToAttribute(GetMagicValueAttribute(), EGameplayModOp::Override, NewValue);
	}

	if (bOutOfMagic && GetMagicValue() > 0.0f)
	{
		bOutOfMagic = false;
	}
}

void UMalogicMagicSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetMagicValueAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMagicValue());
	}
	else if (Attribute == GetMaxMagicValueAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
}
