// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "ViewModelService.generated.h"

class UMVVMViewModelBase;

/** Base type for a business-domain service owned by a ViewModel manager. */
UCLASS(Abstract, Blueprintable)
class MALOGIC_API UViewModelService : public UObject
{
	GENERATED_BODY()

public:
	/** Called by the owning manager after the service is registered. */
	virtual void InitializeService();

	/** Called by the owning manager before the service reference is released. */
	virtual void DeinitializeService();

	/** Finds a ViewModel maintained by this service. */
	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	UMVVMViewModelBase* FindViewModel(FName ViewModelName) const;

	/** Registers a ViewModel created with this service as its outer. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	bool RegisterViewModel(FName ViewModelName, UMVVMViewModelBase* ViewModel);

	/** Removes a ViewModel and releases this service's reference to it. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	bool UnregisterViewModel(FName ViewModelName);

protected:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMVVMViewModelBase>> ViewModels;
};
