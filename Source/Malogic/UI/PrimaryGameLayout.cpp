#include "UI/PrimaryGameLayout.h"

#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "MalogicLogChannels.h"
#include "UI/ActivatableWidgetStack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PrimaryGameLayout)

void UPrimaryGameLayout::NativeConstruct()
{
	Super::NativeConstruct();
	UpdateCurrentTopWidget();
	RefreshInputMode();
}

void UPrimaryGameLayout::UpdateCurrentTopWidget()
{
	if (InteractableUILayerStack->GetTopWidget())
	{
		CurrentTopWidget = InteractableUILayerStack->GetTopWidget();
	}
	else if (HUDLayerStack->GetTopWidget())
	{
		CurrentTopWidget = HUDLayerStack->GetTopWidget();
	}
	else
	{
		CurrentTopWidget = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("No Active widget in PrimaryGameLayout!"));
	}
}

bool UPrimaryGameLayout::PushWidgetToLayer(EWidgetLayer WidgetLayer, UActivatableWidget* Widget)
{
	UE_LOG(LogUI, Log, TEXT("PrimaryGameLayout [%s] pushing widget [%s] to layer [%d]."), *GetNameSafe(this), *GetNameSafe(Widget), static_cast<uint8>(WidgetLayer));
	UActivatableWidgetStack* Stack = nullptr;
	switch (WidgetLayer)
	{
	case EWidgetLayer::HUD: Stack = HUDLayerStack; break;
	case EWidgetLayer::InteractableUI: Stack = InteractableUILayerStack; break;
	case EWidgetLayer::TopUI: Stack = TopUILayerStack; break;
	default: break;
	}

	if (!Stack || !Stack->PushWidget(Widget))
	{
		UE_LOG(LogUI, Warning, TEXT("PrimaryGameLayout [%s] rejected widget [%s] for layer [%d]."), *GetNameSafe(this), *GetNameSafe(Widget), static_cast<uint8>(WidgetLayer));
		return false;
	}

	if (WidgetLayer != EWidgetLayer::TopUI)
	{
		UpdateCurrentTopWidget();
		RefreshInputMode();
	}
	return true;
}

bool UPrimaryGameLayout::PopWidgetFromLayer(EWidgetLayer WidgetLayer)
{
	UActivatableWidgetStack* Stack = nullptr;
	switch (WidgetLayer)
	{
	case EWidgetLayer::HUD: Stack = HUDLayerStack; break;
	case EWidgetLayer::InteractableUI: Stack = InteractableUILayerStack; break;
	case EWidgetLayer::TopUI: Stack = TopUILayerStack; break;
	default: break;
	}

	if (!Stack || !Stack->PopWidget())
	{
		return false;
	}

	if (WidgetLayer != EWidgetLayer::TopUI)
	{
		UpdateCurrentTopWidget();
		RefreshInputMode();
	}
	return true;
}

void UPrimaryGameLayout::RefreshInputMode()
{
	if (!OwningPlayerController)
	{
		return;
	}

	if (!CurrentTopWidget)
	{
		FInputModeGameOnly InputMode;
		OwningPlayerController->SetInputMode(InputMode);
		OwningPlayerController->bShowMouseCursor = false;
		return;
	}

	const FWidgetInputModeConfig Config = CurrentTopWidget->GetInputModeConfig();
	switch (Config.InputMode)
	{
	case EWidgetInputMode::GameOnly:
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(Config.bLockMouseToViewport);
		OwningPlayerController->SetInputMode(InputMode);
		break;
	}
	case EWidgetInputMode::GameAndUI:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(CurrentTopWidget->GetCachedWidget());
		InputMode.SetLockMouseToViewportBehavior(Config.bLockMouseToViewport ? EMouseLockMode::LockAlways : EMouseLockMode::DoNotLock);
		OwningPlayerController->SetInputMode(InputMode);
		break;
	}
	case EWidgetInputMode::UIOnly:
	{
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(CurrentTopWidget->GetCachedWidget());
		InputMode.SetLockMouseToViewportBehavior(Config.bLockMouseToViewport ? EMouseLockMode::LockAlways : EMouseLockMode::DoNotLock);
		OwningPlayerController->SetInputMode(InputMode);
		break;
	}
	}
	OwningPlayerController->bShowMouseCursor = Config.bShowMouseCursor;
}
