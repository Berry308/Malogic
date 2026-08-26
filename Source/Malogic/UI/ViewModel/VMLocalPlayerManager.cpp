// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/VMLocalPlayerManager.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VMLocalPlayerManager)

void UVMLocalPlayerManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReceiveManagerInitialized();

	if (GetLocalPlayer())
	{
		ReceivePlayerControllerChanged(GetLocalPlayer()->GetPlayerController(GetWorld()));
	}
}

void UVMLocalPlayerManager::Deinitialize()
{
	ReceiveManagerDeinitialized();
	DeinitializeServices();
	Super::Deinitialize();
}

void UVMLocalPlayerManager::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	ReceivePlayerControllerChanged(NewPlayerController);
}

UViewModelService* UVMLocalPlayerManager::FindServiceByClass(TSubclassOf<UViewModelService> ServiceClass) const
{
	if (!ServiceClass)
	{
		return nullptr;
	}

	for (const TPair<FName, TObjectPtr<UViewModelService>>& ServiceEntry : Services)
	{
		if (UViewModelService* Service = ServiceEntry.Value.Get(); IsValid(Service) && Service->IsA(ServiceClass))
		{
			return Service;
		}
	}

	return nullptr;
}

UViewModelService* UVMLocalPlayerManager::FindService(FName ServiceName) const
{
	if (const TObjectPtr<UViewModelService>* FoundService = Services.Find(ServiceName))
	{
		return IsValid(FoundService->Get()) ? FoundService->Get() : nullptr;
	}

	return nullptr;
}

bool UVMLocalPlayerManager::RegisterService(FName ServiceName, UViewModelService* Service)
{
	if (ServiceName.IsNone() || !IsValid(Service) || Service->GetOuter() != this)
	{
		return false;
	}

	if (const TObjectPtr<UViewModelService>* ExistingService = Services.Find(ServiceName))
	{
		return ExistingService->Get() == Service;
	}

	Services.Add(ServiceName, Service);
	Service->InitializeService();
	return true;
}

bool UVMLocalPlayerManager::UnregisterService(FName ServiceName)
{
	TObjectPtr<UViewModelService>* Service = Services.Find(ServiceName);
	if (!Service)
	{
		return false;
	}

	if (IsValid(Service->Get()))
	{
		Service->Get()->DeinitializeService();
	}

	Services.Remove(ServiceName);
	return true;
}

void UVMLocalPlayerManager::DeinitializeServices()
{
	for (auto It = Services.CreateIterator(); It; )
	{
		if (UViewModelService* Service = It.Value().Get())
		{
			Service->DeinitializeService();
		}
		It.RemoveCurrent();
	}
}
