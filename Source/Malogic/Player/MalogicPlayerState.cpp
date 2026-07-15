// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/MalogicPlayerState.h"
#include "AbilitySystem/Attributes/MalogicCombatSet.h"
#include "AbilitySystem/Attributes/MalogicHealthSet.h"
#include "AbilitySystem/AbilitySet.h"
#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Character/MalogicPawnData.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "GameFramework/Pawn.h"
#include "MalogicLogChannels.h"
#include "MalogicPlayerController.h"
#include "Net/UnrealNetwork.h"


AMalogicPlayerState::AMalogicPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UMalogicAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// These attribute sets will be detected by AbilitySystemComponent::InitializeComponent. Keeping a reference so that the sets don't get garbage collected before that.
	// 这里的AttributeSet会被自动识别并且添加到ASC中
	HealthSet = CreateDefaultSubobject<UMalogicHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UMalogicCombatSet>(TEXT("CombatSet"));

	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);

}

void AMalogicPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AMalogicPlayerState::Reset()
{
	Super::Reset();
}

void AMalogicPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
}

void AMalogicPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	//@TODO: Copy stats
}

void AMalogicPlayerState::OnDeactivated()
{
}

void AMalogicPlayerState::OnReactivated()
{

}


void AMalogicPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, PawnData);
}

AMalogicPlayerController* AMalogicPlayerState::GetMalogicPlayerController() const
{
	return Cast<AMalogicPlayerController>(GetOwner());
}

UAbilitySystemComponent* AMalogicPlayerState::GetAbilitySystemComponent() const
{
	return GetMalogicAbilitySystemComponent();
}

void AMalogicPlayerState::SetPawnData(const UMalogicPawnData* InPawnData)
{
	check(InPawnData);

	if (!HasAuthority())
	{
		return;
	}

	if (PawnData)
	{
		if (PawnData != InPawnData)
		{
			UE_LOG(LogMalogic, Error, TEXT("Trying to replace PawnData [%s] on PlayerState [%s] that already has PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		}
		return;
	}

	PawnData = InPawnData;
	for (const UAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}

	if (APawn* Pawn = GetPawn())
	{
		if (UMalogicPawnExtensionComponent* PawnExtensionComp = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtensionComp->SetPawnData(PawnData);
		}
	}

	ForceNetUpdate();
}

void AMalogicPlayerState::OnRep_PawnData()
{
	if (APawn* Pawn = GetPawn())
	{
		if (UMalogicPawnExtensionComponent* PawnExtension = UMalogicPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			PawnExtension->HandlePlayerStateReplicated();
		}
	}
}

//初始化ASC->InitAbilityActorInfo
void AMalogicPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn());
}

void AMalogicPlayerState::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AMalogicPlayerState::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AMalogicPlayerState::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AMalogicPlayerState::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}
