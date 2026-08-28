// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "TimerManager.h"
#include "UI/PrimaryGameLayout.h"

#include "MalogicUIManager.generated.h"

class UActivatableWidget;
class UViewModelService;
class APlayerController;

/** Configuration for a widget owned by one local player's UI manager. */
USTRUCT(BlueprintType)
struct FMalogicUIWidgetConfig
{
	GENERATED_BODY()

	/** Stable name used to find or show the created widget. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI")
	FName WidgetName;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI")
	TSubclassOf<UActivatableWidget> WidgetClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI")
	EWidgetLayer WidgetLayer = EWidgetLayer::HUD;

	/** Leave all ViewModel fields empty when this widget does not require a ViewModel. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ViewModelServiceName;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ViewModelName;

	/** Name of the Widget Blueprint's Manual ViewModel slot. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ManualViewModelName;
};

/** Local-player UI lifecycle manager. */
UCLASS(Config = Game, DefaultConfig)
class MALOGIC_API UMalogicUIManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	/** Creates and pushes the configured widget when its local UI prerequisites are ready. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool ShowConfiguredWidget(FName WidgetName);

	/** Pops the top widget from a layer owned by this local player. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PopWidgetFromLayer(EWidgetLayer WidgetLayer);

	UFUNCTION(BlueprintPure, Category = "Malogic|UI")
	UActivatableWidget* FindManagedWidget(FName WidgetName) const;

private:
	bool IsWidgetConfigValid(const FMalogicUIWidgetConfig& WidgetConfig);
	bool TryCreateConfiguredWidget(const FMalogicUIWidgetConfig& WidgetConfig);
	bool InjectViewModel(UActivatableWidget* Widget, const FMalogicUIWidgetConfig& WidgetConfig) const;
	UViewModelService* FindViewModelService(FName ServiceName) const;
	bool HasPendingPersistentWidgets() const;
	void TryCreateConfiguredWidgets();
	void StartWaitingForPrerequisites();
	void StopWaitingForPrerequisites();

	/** Widgets created automatically after their Controller, HUD, layout, and ViewModel prerequisites are ready. */
	UPROPERTY(Config, EditAnywhere, Category = "Malogic|UI")
	TArray<FMalogicUIWidgetConfig> PersistentWidgetConfigs;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UActivatableWidget>> ManagedWidgets;

	TSet<FName> LoggedInvalidWidgetConfigs;

	FTimerHandle PrerequisiteRetryTimer;
};
