// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularPlayerController.h"
#include "MalogicPlayerController.generated.h"

class AMalogicPlayerState;
class APawn;
class APlayerState;
class UMalogicAbilitySystemComponent;
class UPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMalogicPlayerStateChangedEvent);
//class UUserWidget;

/**
 * The base player controller class used by this project.
 */
UCLASS()
class MALOGIC_API AMalogicPlayerController : public AModularPlayerController
{
	GENERATED_BODY()
	
public:
	AMalogicPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Malogic|PlayerController")
	AMalogicPlayerState* GetMalogicPlayerState() const;

	UFUNCTION(BlueprintCallable, Category = "Malogic|PlayerController")
	UMalogicAbilitySystemComponent* GetMalogicAbilitySystemComponent() const;

	/** Notification for Lua/C++ systems that mirror the current PlayerState. */
	UPROPERTY(BlueprintAssignable, Category = "Malogic|PlayerController")
	FMalogicPlayerStateChangedEvent OnMalogicPlayerStateChanged;

	// UI input helpers are kept for later use, but intentionally disabled for now.
	//UFUNCTION(BlueprintCallable, Category = "Malogic|PlayerController")
	//void SetUIInputMode(UUserWidget* WidgetToFocus);

	//UFUNCTION(BlueprintCallable, Category = "Malogic|PlayerController")
	//void SetGameInputMode();

	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

	//~APlayerController interface
	virtual void ReceivedPlayer() override;
	virtual void SetPlayer(UPlayer* InPlayer) override;
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface

protected:
	// Called when the player state is set or cleared.
	virtual void OnPlayerStateChanged();

private:
	void BroadcastOnPlayerStateChanged();

private:

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
