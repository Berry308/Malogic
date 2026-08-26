// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/PlayerVMService.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PlayerVMService)

void UPlayerVMService::InitializeService()
{
	Super::InitializeService();
}

void UPlayerVMService::DeinitializeService()
{
	ReceiveServiceDeinitialized();
	PlayerController = nullptr;

	Super::DeinitializeService();
}

void UPlayerVMService::SetPlayerController(APlayerController* InPlayerController)
{
	PlayerController = InPlayerController;
	ReceivePlayerControllerChanged(InPlayerController);
}
