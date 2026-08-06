#include "Character/MalogicMagicComponent.h"

#include "AbilitySystem/Attributes/MalogicMagicSet.h"
#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "GameFramework/Pawn.h"
#include "MalogicLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicMagicComponent)

UMalogicMagicComponent::UMalogicMagicComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UMalogicMagicComponent::OnRegister()
{
	Super::OnRegister();

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
	{
		UE_LOG(LogMalogic, Error, TEXT("MalogicMagicComponent [%s] must be added to a Pawn."), *GetNameSafe(this));
		return;
	}

	UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn);
	if (!PawnExtension)
	{
		UE_LOG(LogMalogic, Error, TEXT("Cannot initialize MalogicMagicComponent for pawn [%s]: no pawn extension component was found."), *GetNameSafe(Pawn));
		return;
	}

	const FSimpleMulticastDelegate::FDelegate TryInitializeDelegate = FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::TryInitializeWithAbilitySystem);
	PawnExtension->OnAbilitySystemInitialized_RegisterAndCall(TryInitializeDelegate);
	PawnExtension->OnPawnDataInitialized_RegisterAndCall(TryInitializeDelegate);
	PawnExtension->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::UninitializeFromAbilitySystem));
}

void UMalogicMagicComponent::OnUnregister()
{
	UninitializeFromAbilitySystem();
	Super::OnUnregister();
}

void UMalogicMagicComponent::TryInitializeWithAbilitySystem()
{
	if (AbilitySystemComponent)
	{
		return;
	}

	const UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(GetOwner());
	if (!PawnExtension)
	{
		return;
	}

	UMalogicAbilitySystemComponent* PawnAbilitySystemComponent = PawnExtension->GetMalogicAbilitySystemComponent();
	if (!PawnAbilitySystemComponent || !PawnAbilitySystemComponent->GetSet<UMalogicMagicSet>())
	{
		return;
	}

	InitializeWithAbilitySystem(PawnAbilitySystemComponent);
}

void UMalogicMagicComponent::InitializeWithAbilitySystem(UMalogicAbilitySystemComponent* InAbilitySystemComponent)
{
	AActor* Owner = GetOwner();
	check(Owner);

	if (AbilitySystemComponent)
	{
		UE_LOG(LogMalogic, Error, TEXT("MalogicMagicComponent for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}

	AbilitySystemComponent = InAbilitySystemComponent;
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogMalogic, Error, TEXT("Cannot initialize MalogicMagicComponent for owner [%s] with a null ability system."), *GetNameSafe(Owner));
		return;
	}

	MagicSet = AbilitySystemComponent->GetSet<UMalogicMagicSet>();
	if (!MagicSet)
	{
		UE_LOG(LogMalogic, Error, TEXT("Cannot initialize MalogicMagicComponent for owner [%s]: the ability system has no magic set."), *GetNameSafe(Owner));
		AbilitySystemComponent = nullptr;
		return;
	}

	MagicSet->OnMagicValueChanged.AddUObject(this, &ThisClass::HandleMagicValueChanged);
	MagicSet->OnMaxMagicValueChanged.AddUObject(this, &ThisClass::HandleMaxMagicValueChanged);
	MagicSet->OnOutOfMagic.AddUObject(this, &ThisClass::HandleOutOfMagic);

	AbilitySystemComponent->SetNumericAttributeBase(UMalogicMagicSet::GetMagicValueAttribute(), MagicSet->GetMaxMagicValue());

	OnMagicValueChanged.Broadcast(this, MagicSet->GetMagicValue(), MagicSet->GetMagicValue(), nullptr);
	OnMaxMagicValueChanged.Broadcast(this, MagicSet->GetMaxMagicValue(), MagicSet->GetMaxMagicValue(), nullptr);
}

void UMalogicMagicComponent::UninitializeFromAbilitySystem()
{
	if (MagicSet)
	{
		MagicSet->OnMagicValueChanged.RemoveAll(this);
		MagicSet->OnMaxMagicValueChanged.RemoveAll(this);
		MagicSet->OnOutOfMagic.RemoveAll(this);
	}

	MagicSet = nullptr;
	AbilitySystemComponent = nullptr;
}

float UMalogicMagicComponent::GetMagicValue() const
{
	return MagicSet ? MagicSet->GetMagicValue() : 0.0f;
}

float UMalogicMagicComponent::GetMaxMagicValue() const
{
	return MagicSet ? MagicSet->GetMaxMagicValue() : 0.0f;
}

float UMalogicMagicComponent::GetMagicValueNormalized() const
{
	const float MaxMagicValue = GetMaxMagicValue();
	return MaxMagicValue > 0.0f ? GetMagicValue() / MaxMagicValue : 0.0f;
}

void UMalogicMagicComponent::HandleMagicValueChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue)
{
	OnMagicValueChanged.Broadcast(this, OldValue, NewValue, EffectInstigator);
}

void UMalogicMagicComponent::HandleMaxMagicValueChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue)
{
	OnMaxMagicValueChanged.Broadcast(this, OldValue, NewValue, EffectInstigator);
}

void UMalogicMagicComponent::HandleOutOfMagic(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue)
{
	OnOutOfMagic.Broadcast(this, OldValue, NewValue, EffectInstigator);
}
