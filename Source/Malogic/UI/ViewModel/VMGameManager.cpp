// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/VMGameManager.h"

#include "MalogicLogChannels.h"

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
		if (IsValid(FoundService->Get()))
		{
			return FoundService->Get();
		}

		UE_LOG(LogUI, Warning, TEXT("Game ViewModel manager [%s] has an invalid Service registered under name [%s]."), *GetNameSafe(this), *ServiceName.ToString());
		return nullptr;
	}

	UE_LOG(LogUI, Warning, TEXT("Game ViewModel manager [%s] could not find Service [%s]."), *GetNameSafe(this), *ServiceName.ToString());
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
	TObjectPtr<UViewModelService> Service = nullptr;
	if (!Services.RemoveAndCopyValue(ServiceName, Service))
	{
		return false;
	}

	if (IsValid(Service.Get()))
	{
		Service->DeinitializeService();
	}

	return true;
}

void UVMGameManager::DeinitializeServices()
{
	// Remove each entry before invoking callbacks so reentrant Lua code cannot invalidate a live map iterator.
	while (Services.Num() > 0)
	{
		FName ServiceName;
		if (const auto It = Services.CreateConstIterator(); It)
		{
			ServiceName = It.Key();
		}

		TObjectPtr<UViewModelService> Service = nullptr;
		if (!Services.RemoveAndCopyValue(ServiceName, Service))
		{
			break;
		}

		if (IsValid(Service.Get()))
		{
			Service->DeinitializeService();
		}
	}
}
