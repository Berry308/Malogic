// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnLuaInterface.h"
#include "UI/ViewModel/ViewModelService.h"

#include "VMWorldManager.generated.h"

/** World lifetime container for level and map ViewModel services. */
UCLASS()
class MALOGIC_API UVMWorldManager : public UWorldSubsystem, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual FString GetModuleName_Implementation() const override
	{
		return TEXT("Malogic.UI.ViewModel.VMWorldManager");
	}

	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	UViewModelService* FindServiceByClass(TSubclassOf<UViewModelService> ServiceClass) const;

	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	UViewModelService* FindService(FName ServiceName) const;

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
