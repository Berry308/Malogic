// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnLuaInterface.h"
#include "UI/ViewModel/ViewModelService.h"

#include "VMGameManager.generated.h"

/** Game-instance lifetime container for global ViewModel services. */
UCLASS()
class MALOGIC_API UVMGameManager : public UGameInstanceSubsystem, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual FString GetModuleName_Implementation() const override
	{
		return TEXT("Malogic.UI.ViewModel.VMGameManager");
	}

	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	UViewModelService* FindServiceByClass(TSubclassOf<UViewModelService> ServiceClass) const;

	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	UViewModelService* FindService(FName ServiceName) const;

	/** Registers a service created with this manager as its outer. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	bool RegisterService(FName ServiceName, UViewModelService* Service);

	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	bool UnregisterService(FName ServiceName);

protected:
	/** Implemented by Lua to create and register services for this manager scope. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveManagerInitialized();

	/** Implemented by Lua to release manager-side Lua state. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveManagerDeinitialized();

private:
	void DeinitializeServices();

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UViewModelService>> Services;
};
