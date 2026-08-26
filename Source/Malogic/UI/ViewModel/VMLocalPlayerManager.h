// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UnLuaInterface.h"
#include "UI/ViewModel/ViewModelService.h"

#include "VMLocalPlayerManager.generated.h"

class APlayerController;

/** Local-player lifetime container for player-scoped ViewModel services. */
UCLASS()
class MALOGIC_API UVMLocalPlayerManager : public ULocalPlayerSubsystem, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	virtual FString GetModuleName_Implementation() const override
	{
		return TEXT("Malogic.UI.ViewModel.VMLocalPlayerManager");
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
	/** Implemented by Lua to create and register services for the local-player scope. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveManagerInitialized();

	/** Implemented by Lua to release manager-side Lua state. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveManagerDeinitialized();

	/** Implemented by Lua to forward the current controller to interested services. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceivePlayerControllerChanged(APlayerController* NewPlayerController);

private:
	void DeinitializeServices();

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UViewModelService>> Services;
};
