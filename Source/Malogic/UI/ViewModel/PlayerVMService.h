// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UnLuaInterface.h"
#include "UI/ViewModel/ViewModelService.h"

#include "PlayerVMService.generated.h"

class APlayerController;

/** C++ lifecycle and storage bridge for the Lua player ViewModel service. */
UCLASS(BlueprintType)
class MALOGIC_API UPlayerVMService : public UViewModelService, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	virtual void InitializeService() override;
	virtual void DeinitializeService() override;

	virtual FString GetModuleName_Implementation() const override
	{
		return TEXT("Malogic.UI.ViewModel.PlayerVMService");
	}

	/** Stores the current controller and notifies the Lua service to refresh its bindings. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|ViewModel")
	void SetPlayerController(APlayerController* InPlayerController);

	UFUNCTION(BlueprintPure, Category = "Malogic|ViewModel")
	APlayerController* GetPlayerController() const { return PlayerController; }

protected:
	/** Implemented by Lua after the service has been registered and fully initialized. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveServiceInitialized();

	/** Implemented by Lua to bind or rebind the supplied controller and its current Pawn. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceivePlayerControllerChanged(APlayerController* NewPlayerController);

	/** Implemented by Lua to remove all delegate bindings before service destruction. */
	UFUNCTION(BlueprintImplementableEvent)
	void ReceiveServiceDeinitialized();

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Malogic|ViewModel", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<APlayerController> PlayerController;
};
