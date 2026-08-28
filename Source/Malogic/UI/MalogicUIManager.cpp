// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MalogicUIManager.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "FieldNotification/IFieldValueChanged.h"
#include "GameFramework/PlayerController.h"
#include "MalogicLogChannels.h"
#include "MVVMSubsystem.h"
#include "MVVMViewModelBase.h"
#include "UI/ActivatableWidget.h"
#include "UI/ViewModel/VMLocalPlayerManager.h"
#include "UI/ViewModel/ViewModelService.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicUIManager)

void UMalogicUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UVMLocalPlayerManager>();
	Super::Initialize(Collection);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UVMLocalPlayerManager* ViewModelManager = LocalPlayer->GetSubsystem<UVMLocalPlayerManager>())
		{
			ServiceRegisteredHandle = ViewModelManager->OnServiceRegistered().AddUObject(this, &ThisClass::HandleViewModelServiceRegistered);
		}
	}

	RegisterHUD(GetCurrentHUD());
}

void UMalogicUIManager::Deinitialize()
{
	UnbindFromHUD();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UVMLocalPlayerManager* ViewModelManager = LocalPlayer->GetSubsystem<UVMLocalPlayerManager>())
		{
			ViewModelManager->OnServiceRegistered().Remove(ServiceRegisteredHandle);
		}
	}
	ServiceRegisteredHandle.Reset();
	ManagedWidgets.Reset();

	Super::Deinitialize();
}

void UMalogicUIManager::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	RegisterHUD(NewPlayerController ? Cast<AMalogicHUD>(NewPlayerController->GetHUD()) : nullptr);
}

void UMalogicUIManager::RegisterHUD(AMalogicHUD* HUD)
{
	if (BoundHUD.IsValid() && BoundHUD.Get() == HUD)
	{
		return;
	}

	UnbindFromHUD();
	ManagedWidgets.Reset();

	if (!HUD || HUD != GetCurrentHUD())
	{
		return;
	}

	BindToHUD(HUD);
}

bool UMalogicUIManager::ShowDefaultWidget(FName WidgetName)
{
	AMalogicHUD* HUD = BoundHUD.Get();
	if (!HUD || !HUD->IsHUDReady())
	{
		return false;
	}

	const FMalogicHUDWidgetConfig* WidgetConfig = HUD->GetDefaultWidgetConfigs().FindByPredicate(
		[WidgetName](const FMalogicHUDWidgetConfig& Candidate)
		{
			return Candidate.WidgetName == WidgetName;
		});
	return WidgetConfig && TryCreateDefaultWidget(*WidgetConfig);
}

bool UMalogicUIManager::PopWidgetFromLayer(EWidgetLayer WidgetLayer)
{
	AMalogicHUD* HUD = BoundHUD.Get();
	return HUD && HUD->RemoveTopWidgetFrom(WidgetLayer);
}

UActivatableWidget* UMalogicUIManager::FindManagedWidget(FName WidgetName) const
{
	const TObjectPtr<UActivatableWidget>* FoundWidget = ManagedWidgets.Find(WidgetName);
	return FoundWidget && IsValid(FoundWidget->Get()) ? FoundWidget->Get() : nullptr;
}

void UMalogicUIManager::HandleHUDReady(AMalogicHUD* ReadyHUD)
{
	if (ReadyHUD && ReadyHUD == BoundHUD.Get())
	{
		TryCreateDefaultWidgets();
	}
}

void UMalogicUIManager::HandleViewModelServiceRegistered(FName /*ServiceName*/, UViewModelService* /*Service*/)
{
	if (BoundHUD.IsValid() && BoundHUD->IsHUDReady())
	{
		TryCreateDefaultWidgets();
	}
}

void UMalogicUIManager::BindToHUD(AMalogicHUD* HUD)
{
	if (BoundHUD.Get() == HUD)
	{
		return;
	}

	UnbindFromHUD();
	if (!HUD)
	{
		return;
	}

	BoundHUD = HUD;
	HUD->OnHUDReady.AddDynamic(this, &ThisClass::HandleHUDReady);
	if (HUD->IsHUDReady())
	{
		HandleHUDReady(HUD);
	}
}

void UMalogicUIManager::UnbindFromHUD()
{
	if (BoundHUD.IsValid())
	{
		BoundHUD->OnHUDReady.RemoveDynamic(this, &ThisClass::HandleHUDReady);
	}
	BoundHUD.Reset();
}

void UMalogicUIManager::TryCreateDefaultWidgets()
{
	AMalogicHUD* HUD = BoundHUD.Get();
	if (!HUD || !HUD->IsHUDReady())
	{
		return;
	}

	for (const FMalogicHUDWidgetConfig& WidgetConfig : HUD->GetDefaultWidgetConfigs())
	{
		TryCreateDefaultWidget(WidgetConfig);
	}
}

bool UMalogicUIManager::TryCreateDefaultWidget(const FMalogicHUDWidgetConfig& WidgetConfig)
{
	if (!IsWidgetConfigValid(WidgetConfig))
	{
		return false;
	}

	if (FindManagedWidget(WidgetConfig.WidgetName))
	{
		return true;
	}

	AMalogicHUD* HUD = BoundHUD.Get();
	APlayerController* PlayerController = HUD ? HUD->GetOwningPlayerController() : nullptr;
	if (!HUD || !HUD->IsHUDReady() || !PlayerController)
	{
		return false;
	}

	UActivatableWidget* Widget = CreateWidget<UActivatableWidget>(PlayerController, WidgetConfig.WidgetClass);
	if (!Widget || !InjectViewModel(Widget, WidgetConfig)) return false;

	if (!HUD->AddWidgetToLayer(WidgetConfig.WidgetLayer, Widget)) return false;

	ManagedWidgets.Add(WidgetConfig.WidgetName, Widget);
	return true;
}

bool UMalogicUIManager::IsWidgetConfigValid(const FMalogicHUDWidgetConfig& WidgetConfig)
{
	const bool bHasAnyViewModelField = !WidgetConfig.ViewModelServiceName.IsNone()
		|| !WidgetConfig.ViewModelName.IsNone()
		|| !WidgetConfig.ManualViewModelName.IsNone();
	const bool bHasCompleteViewModelConfig = !WidgetConfig.ViewModelServiceName.IsNone()
		&& !WidgetConfig.ViewModelName.IsNone()
		&& !WidgetConfig.ManualViewModelName.IsNone();
	const bool bIsValid = !WidgetConfig.WidgetName.IsNone()
		&& WidgetConfig.WidgetClass
		&& (!bHasAnyViewModelField || bHasCompleteViewModelConfig);
	if (!bIsValid)
	{
		UE_LOG(LogUI, Error, TEXT("HUD widget config [%s] requires WidgetName and WidgetClass. ViewModel fields must be either all set or all empty."), *WidgetConfig.WidgetName.ToString());
		return false;
	}

	return true;
}

bool UMalogicUIManager::InjectViewModel(UActivatableWidget* Widget, const FMalogicHUDWidgetConfig& WidgetConfig) const
{
	if (WidgetConfig.ViewModelServiceName.IsNone())
	{
		return true;
	}

	UViewModelService* Service = FindViewModelService(WidgetConfig.ViewModelServiceName);
	UMVVMViewModelBase* ViewModel = Service ? Service->FindViewModel(WidgetConfig.ViewModelName) : nullptr;
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(Widget);
	if (!View || !ViewModel)
	{
		return false;
	}

	TScriptInterface<INotifyFieldValueChanged> ViewModelInterface;
	ViewModelInterface.SetObject(ViewModel);
	ViewModelInterface.SetInterface(Cast<INotifyFieldValueChanged>(ViewModel));
	return ViewModelInterface.GetInterface() && View->SetViewModel(WidgetConfig.ManualViewModelName, ViewModelInterface);
}

UViewModelService* UMalogicUIManager::FindViewModelService(FName ServiceName) const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UVMLocalPlayerManager* ViewModelManager = LocalPlayer ? LocalPlayer->GetSubsystem<UVMLocalPlayerManager>() : nullptr;
	return ViewModelManager ? ViewModelManager->FindService(ServiceName) : nullptr;
}

AMalogicHUD* UMalogicUIManager::GetCurrentHUD() const
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	return PlayerController ? Cast<AMalogicHUD>(PlayerController->GetHUD()) : nullptr;
}
