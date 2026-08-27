#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PrimaryGameLayout.generated.h"

UENUM(BlueprintType)
enum class EWidgetLayer : uint8
{
	HUD,
	InteractableUI,
	TopUI
};

class AHUD;
class APlayerController;
class UActivatableWidget;
class UActivatableWidgetStack;
class UOverlay;

UCLASS()
class MALOGIC_API UPrimaryGameLayout : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PushWidgetToLayer(EWidgetLayer WidgetLayer, UActivatableWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PopWidgetFromLayer(EWidgetLayer WidgetLayer);

	void SetOwningHUD(AHUD* NewOwningHUD) { OwningHUD = NewOwningHUD; }
	void SetOwningPlayerController(APlayerController* NewOwningPlayerController) { OwningPlayerController = NewOwningPlayerController; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UActivatableWidgetStack> HUDLayerStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UActivatableWidgetStack> InteractableUILayerStack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UActivatableWidgetStack> TopUILayerStack;

private:
	void UpdateCurrentTopWidget();
	void RefreshInputMode();

	UPROPERTY(Transient)
	TObjectPtr<UActivatableWidget> CurrentTopWidget;

	UPROPERTY(Transient)
	TObjectPtr<AHUD> OwningHUD;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> OwningPlayerController;
};
