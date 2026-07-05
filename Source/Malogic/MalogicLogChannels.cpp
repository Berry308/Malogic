// Fill out your copyright notice in the Description page of Project Settings.

#include "MalogicLogChannels.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogMalogic);
DEFINE_LOG_CATEGORY(LogMRExperience);
DEFINE_LOG_CATEGORY(LogMRAbilitySystem);
DEFINE_LOG_CATEGORY(LogMRTeams);
DEFINE_LOG_CATEGORY(LogAI);
DEFINE_LOG_CATEGORY(LogUI);

FString GetClientServerContextString(UObject* ContextObject)
{
	ENetRole Role = ROLE_None;

	if (AActor* Actor = Cast<AActor>(ContextObject))
	{
		Role = Actor->GetLocalRole();
	}
	else if (UActorComponent* Component = Cast<UActorComponent>(ContextObject))
	{
		Role = Component->GetOwnerRole();
	}

	if (Role != ROLE_None)
	{
		return (Role == ROLE_Authority) ? TEXT("Server") : TEXT("Client");
	}
	else
	{
#if WITH_EDITOR
		if (GIsEditor)
		{
			extern ENGINE_API FString GPlayInEditorContextString;
			return GPlayInEditorContextString;
		}
#endif
	}

	return TEXT("[]");
}
