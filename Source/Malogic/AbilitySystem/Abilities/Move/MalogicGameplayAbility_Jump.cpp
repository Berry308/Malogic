// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Move/MalogicGameplayAbility_Jump.h"

#include "Abilities/Tasks/AbilityTask_StartAbilityState.h"
#include "Character/MalogicCharacter.h"
#include "UnLua.h"
#include "MalogicLogChannels.h"

UMalogicGameplayAbility_Jump::UMalogicGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UMalogicGameplayAbility_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
        
    // 调用 Lua 脚本中的激活逻辑
    // 假设使用了 UnLua 插件提供的逻辑分发
    if (this->GetClass()->ImplementsInterface(UUnLuaInterface::StaticClass()))
    {
		UE_LOG(LogMRAbilitySystem, Log, TEXT("Call Lua"));
		ReceiveOnAbilityActivated();
	}
}

void UMalogicGameplayAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	CharacterJumpStop();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


void UMalogicGameplayAbility_Jump::CharacterJumpStart()
{
	if (AMalogicCharacter* MalogicCharacter = GetMalogicCharacterFromActorInfo())
	{
		if (MalogicCharacter->IsLocallyControlled() && !MalogicCharacter->bPressedJump)
		{
			MalogicCharacter->UnCrouch();
			MalogicCharacter->Jump();
		}
	}
}

void UMalogicGameplayAbility_Jump::CharacterJumpStop()
{
	if (AMalogicCharacter* MalogicCharacter = GetMalogicCharacterFromActorInfo())
	{
		if (MalogicCharacter->IsLocallyControlled() && MalogicCharacter->bPressedJump)
		{
			MalogicCharacter->StopJumping();
		}
	}
}

void UMalogicGameplayAbility_Jump::OnInputReleasedLua()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}
