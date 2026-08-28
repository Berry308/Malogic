// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MalogicUIManager.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "FieldNotification/IFieldValueChanged.h"
#include "GameFramework/PlayerController.h"
#include "MalogicLogChannels.h"
#include "MVVMSubsystem.h"
#include "MVVMViewModelBase.h"
#include "TimerManager.h"
#include "UI/ActivatableWidget.h"
#include "UI/MalogicHUD.h"
#include "UI/ViewModel/VMLocalPlayerManager.h"
#include "UI/ViewModel/ViewModelService.h"
#include "View/MVVMView.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicUIManager)

void UMalogicUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	StartWaitingForPrerequisites();
}

void UMalogicUIManager::Deinitialize()
{
	StopWaitingForPrerequisites();

	for (const TPair<FName, TObjectPtr<UActivatableWidget>>& WidgetEntry : ManagedWidgets)
	{
		if (UActivatableWidget* Widget = WidgetEntry.Value.Get())
		{
			Widget->RemoveFromParent();
		}
	}
	ManagedWidgets.Reset();

	Super::Deinitialize();
}

void UMalogicUIManager::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	StartWaitingForPrerequisites();
}

bool UMalogicUIManager::ShowConfiguredWidget(FName WidgetName)
{
	const FMalogicUIWidgetConfig* WidgetConfig = PersistentWidgetConfigs.FindByPredicate(
		[WidgetName](const FMalogicUIWidgetConfig& Candidate)
		{
			return Candidate.WidgetName == WidgetName;
		});

	const bool bWidgetCreated = WidgetConfig && TryCreateConfiguredWidget(*WidgetConfig);
	if (!bWidgetCreated)
	{
		StartWaitingForPrerequisites();
	}
	return bWidgetCreated;
}

bool UMalogicUIManager::PopWidgetFromLayer(EWidgetLayer WidgetLayer)
{
	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	AMalogicHUD* HUD = PlayerController ? Cast<AMalogicHUD>(PlayerController->GetHUD()) : nullptr;
	return HUD && HUD->RemoveTopWidgetFrom(WidgetLayer);
}

UActivatableWidget* UMalogicUIManager::FindManagedWidget(FName WidgetName) const
{
	const TObjectPtr<UActivatableWidget>* FoundWidget = ManagedWidgets.Find(WidgetName);
	return FoundWidget && IsValid(FoundWidget->Get()) ? FoundWidget->Get() : nullptr;
}

bool UMalogicUIManager::TryCreateConfiguredWidget(const FMalogicUIWidgetConfig& WidgetConfig)
{
	if (!IsWidgetConfigValid(WidgetConfig))
	{
		return false;
	}

	if (FindManagedWidget(WidgetConfig.WidgetName))
	{
		return true;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* PlayerController = LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
	AMalogicHUD* HUD = PlayerController ? Cast<AMalogicHUD>(PlayerController->GetHUD()) : nullptr;
	if (!PlayerController || !HUD || !HUD->GetPrimaryGameLayout())
	{
		return false;
	}

	UActivatableWidget* Widget = CreateWidget<UActivatableWidget>(PlayerController, WidgetConfig.WidgetClass);
	if (!Widget || !InjectViewModel(Widget, WidgetConfig))
	{
		return false;
	}

	if (!HUD->AddWidgetToLayer(WidgetConfig.WidgetLayer, Widget))
	{
		return false;
	}

	ManagedWidgets.Add(WidgetConfig.WidgetName, Widget);
	return true;
}

bool UMalogicUIManager::IsWidgetConfigValid(const FMalogicUIWidgetConfig& WidgetConfig)
{
	const bool bIsValid = !WidgetConfig.WidgetName.IsNone()
		&& WidgetConfig.WidgetClass
		&& !WidgetConfig.ViewModelServiceName.IsNone()
		&& !WidgetConfig.ViewModelName.IsNone()
		&& !WidgetConfig.ManualViewModelName.IsNone();
	if (bIsValid || LoggedInvalidWidgetConfigs.Contains(WidgetConfig.WidgetName))
	{
		return bIsValid;
	}

	LoggedInvalidWidgetConfigs.Add(WidgetConfig.WidgetName);
	UE_LOG(LogUI, Warning, TEXT("UI widget config [%s] requires WidgetClass, ViewModelServiceName, ViewModelName, and ManualViewModelName."), *WidgetConfig.WidgetName.ToString());
	return false;
}

bool UMalogicUIManager::InjectViewModel(UActivatableWidget* Widget, const FMalogicUIWidgetConfig& WidgetConfig) const
{
	UViewModelService* Service = FindViewModelService(WidgetConfig.ViewModelServiceName);
	UMVVMViewModelBase* ViewModel = Service ? Service->FindViewModel(WidgetConfig.ViewModelName) : nullptr;
	UMVVMView* View = UMVVMSubsystem::GetViewFromUserWidget(Widget);
	if (!View || !ViewModel)
	{
		return false;
	}

	//MVVM 插件并不关心你的 ViewModel 到底是什么类，它唯一关心的能力是：“当你属性改变时，你能通知我”。
	//这个能力是由 INotifyFieldValueChanged 接口定义的。通过使用接口，任何 UObject（即便它不继承自 UMVVMViewModelBase）只要实现了该接口，就能作为数据源注入。
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

//是否有待定的持久化widget
bool UMalogicUIManager::HasPendingPersistentWidgets() const
{
	return PersistentWidgetConfigs.ContainsByPredicate([this](const FMalogicUIWidgetConfig& WidgetConfig)
	{
		return !LoggedInvalidWidgetConfigs.Contains(WidgetConfig.WidgetName) && !FindManagedWidget(WidgetConfig.WidgetName);
	});
}

void UMalogicUIManager::TryCreateConfiguredWidgets()
{
	for (const FMalogicUIWidgetConfig& WidgetConfig : PersistentWidgetConfigs)
	{
		TryCreateConfiguredWidget(WidgetConfig);
	}

	if (!HasPendingPersistentWidgets())
	{
		StopWaitingForPrerequisites();
	}
}

void UMalogicUIManager::StartWaitingForPrerequisites()
{
	TryCreateConfiguredWidgets();

	UWorld* World = GetWorld();
	if (!World || !HasPendingPersistentWidgets() || PrerequisiteRetryTimer.IsValid())
	{
		return;
	}

	World->GetTimerManager().SetTimer(PrerequisiteRetryTimer, this, &UMalogicUIManager::TryCreateConfiguredWidgets, 0.1f, true);
}

void UMalogicUIManager::StopWaitingForPrerequisites()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PrerequisiteRetryTimer);
	}
}
