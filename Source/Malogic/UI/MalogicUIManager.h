// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/MalogicHUD.h"

#include "MalogicUIManager.generated.h"

class UActivatableWidget;
class UViewModelService;
class APlayerController;

/** Local-player UI lifecycle manager. */
UCLASS()
class MALOGIC_API UMalogicUIManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	/** Registers the HUD currently owned by this local player. */
	void RegisterHUD(AMalogicHUD* HUD);

	/** Creates a default widget declared by the bound HUD when all prerequisites are ready. */
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool ShowDefaultWidget(FName WidgetName);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PopWidgetFromLayer(EWidgetLayer WidgetLayer);

	UFUNCTION(BlueprintPure, Category = "Malogic|UI")
	UActivatableWidget* FindManagedWidget(FName WidgetName) const;

private:
	UFUNCTION()
	void HandleHUDReady(AMalogicHUD* ReadyHUD);

	void HandleViewModelServiceRegistered(FName ServiceName, UViewModelService* Service);
	void BindToHUD(AMalogicHUD* HUD);
	void UnbindFromHUD();
	void TryCreateDefaultWidgets();
	bool TryCreateDefaultWidget(const FMalogicHUDWidgetConfig& WidgetConfig);
	bool IsWidgetConfigValid(const FMalogicHUDWidgetConfig& WidgetConfig);
	bool InjectViewModel(UActivatableWidget* Widget, const FMalogicHUDWidgetConfig& WidgetConfig) const;
	UViewModelService* FindViewModelService(FName ServiceName) const;
	AMalogicHUD* GetCurrentHUD() const;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UActivatableWidget>> ManagedWidgets;

	TWeakObjectPtr<AMalogicHUD> BoundHUD;
	FDelegateHandle ServiceRegisteredHandle;
};
