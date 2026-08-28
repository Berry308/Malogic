#include "UI/MalogicHUD.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "MalogicLogChannels.h"
#include "UI/MalogicUIManager.h"
#include "UI/PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicHUD)

void AMalogicHUD::BeginPlay()
{
	Super::BeginPlay();

	if (!PrimaryGameLayoutClass)
	{
		UE_LOG(LogUI, Warning, TEXT("MalogicHUD [%s] has no PrimaryGameLayoutClass configured."), *GetNameSafe(this));
		return;
	}

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		PrimaryGameLayoutInstance = CreateWidget<UPrimaryGameLayout>(PlayerController, PrimaryGameLayoutClass);
		if (PrimaryGameLayoutInstance)
		{
			PrimaryGameLayoutInstance->SetOwningHUD(this);
			PrimaryGameLayoutInstance->SetOwningPlayerController(PlayerController);
			PrimaryGameLayoutInstance->AddToViewport();
			bIsHUDReady = true;

			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (UMalogicUIManager* UIManager = LocalPlayer->GetSubsystem<UMalogicUIManager>())
				{
					UIManager->RegisterHUD(this);
				}
			}

			OnHUDReady.Broadcast(this);
		}
	}
}

bool AMalogicHUD::AddWidgetToLayer(EWidgetLayer WidgetLayer, UActivatableWidget* NewWidget)
{
	return PrimaryGameLayoutInstance && PrimaryGameLayoutInstance->PushWidgetToLayer(WidgetLayer, NewWidget);
}

bool AMalogicHUD::RemoveTopWidgetFrom(EWidgetLayer WidgetLayer)
{
	return PrimaryGameLayoutInstance && PrimaryGameLayoutInstance->PopWidgetFromLayer(WidgetLayer);
}
