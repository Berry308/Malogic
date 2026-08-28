#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/PrimaryGameLayout.h"
#include "MalogicHUD.generated.h"

class UActivatableWidget;
class UPrimaryGameLayout;
class AMalogicHUD;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMalogicHUDReadyDelegate, AMalogicHUD*, ReadyHUD);

USTRUCT(BlueprintType)
struct FMalogicHUDWidgetConfig
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI")
	FName WidgetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI")
	TSubclassOf<UActivatableWidget> WidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI")
	EWidgetLayer WidgetLayer = EWidgetLayer::HUD;

	/** Leave all ViewModel fields empty when this widget has no MVVM bindings. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ViewModelServiceName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ViewModelName;

	/** Name of the Widget Blueprint's Manual ViewModel slot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI|ViewModel")
	FName ManualViewModelName;
};

UCLASS()
class MALOGIC_API AMalogicHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Broadcast after the primary game layout has been created and added to the viewport. */
	UPROPERTY(BlueprintAssignable, Category = "Malogic|UI")
	FMalogicHUDReadyDelegate OnHUDReady;

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool AddWidgetToLayer(EWidgetLayer WidgetLayer, UActivatableWidget* NewWidget);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool RemoveTopWidgetFrom(EWidgetLayer WidgetLayer);

	UPROPERTY(EditDefaultsOnly, Category = "Malogic|UI")
	TSubclassOf<UPrimaryGameLayout> PrimaryGameLayoutClass;

	/** Widgets the local UI manager creates once this HUD is ready. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Malogic|UI")
	TArray<FMalogicHUDWidgetConfig> DefaultWidgetConfigs;

	UPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayoutInstance; }
	bool IsHUDReady() const { return bIsHUDReady; }
	const TArray<FMalogicHUDWidgetConfig>& GetDefaultWidgetConfigs() const { return DefaultWidgetConfigs; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UPrimaryGameLayout> PrimaryGameLayoutInstance;

	bool bIsHUDReady = false;
};
