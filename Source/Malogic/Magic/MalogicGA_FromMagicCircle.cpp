// Fill out your copyright notice in the Description page of Project Settings.


#include "Magic/MalogicGA_FromMagicCircle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGA_FromMagicCircle)

UMalogicGA_FromMagicCircle::UMalogicGA_FromMagicCircle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
}

