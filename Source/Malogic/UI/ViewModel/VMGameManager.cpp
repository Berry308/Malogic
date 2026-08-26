// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/VMGameManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(VMGameManager)

void UVMGameManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReceiveManagerInitialized();
}

void UVMGameManager::Deinitialize()
{
	ReceiveManagerDeinitialized();
	DeinitializeServices();
	Super::Deinitialize();
}

UViewModelService* UVMGameManager::FindServiceByClass(TSubclassOf<UViewModelService> ServiceClass) const
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

UViewModelService* UVMGameManager::FindService(FName ServiceName) const
{
	if (const TObjectPtr<UViewModelService>* FoundService = Services.Find(ServiceName))
	{
		return IsValid(FoundService->Get()) ? FoundService->Get() : nullptr;
	}

	return nullptr;
}

bool UVMGameManager::RegisterService(FName ServiceName, UViewModelService* Service)
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

bool UVMGameManager::UnregisterService(FName ServiceName)
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

void UVMGameManager::DeinitializeServices()
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
