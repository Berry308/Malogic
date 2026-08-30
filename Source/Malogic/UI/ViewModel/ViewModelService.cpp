// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ViewModel/ViewModelService.h"

#include "MalogicLogChannels.h"
#include "MVVMViewModelBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ViewModelService)

void UViewModelService::InitializeService()
{
}

void UViewModelService::DeinitializeService()
{
	ViewModels.Reset();
}

UMVVMViewModelBase* UViewModelService::FindViewModel(FName ViewModelName) const
{
	if (const TObjectPtr<UMVVMViewModelBase>* FoundViewModel = ViewModels.Find(ViewModelName))
	{
		if (IsValid(FoundViewModel->Get()))
		{
			return FoundViewModel->Get();
		}

		UE_LOG(LogUI, Warning, TEXT("ViewModel service [%s] has an invalid ViewModel registered under name [%s]."), *GetNameSafe(this), *ViewModelName.ToString());
		return nullptr;
	}

	UE_LOG(LogUI, Warning, TEXT("ViewModel service [%s] could not find ViewModel [%s]."), *GetNameSafe(this), *ViewModelName.ToString());
	return nullptr;
}

bool UViewModelService::RegisterViewModel(FName ViewModelName, UMVVMViewModelBase* ViewModel)
{
	if (ViewModelName.IsNone() || !IsValid(ViewModel) || ViewModel->GetOuter() != this)
	{
		return false;
	}

	if (const TObjectPtr<UMVVMViewModelBase>* ExistingViewModel = ViewModels.Find(ViewModelName))
	{
		return ExistingViewModel->Get() == ViewModel;
	}

	ViewModels.Add(ViewModelName, ViewModel);
	return true;
}

bool UViewModelService::UnregisterViewModel(FName ViewModelName)
{
	return ViewModels.Remove(ViewModelName) > 0;
}
