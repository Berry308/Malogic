#include "UI/MalogicUISubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/MalogicHUD.h"
#include "UI/ActivatableWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicUISubsystem)

void UMalogicUISubsystem::DeliverWidgetToAllPlayers(EWidgetLayer WidgetLayer, TSubclassOf<UActivatableWidget> WidgetClass)
{
	if (!WidgetClass || !GetWorld())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMalogicHUD* HUD = It->Get() ? Cast<AMalogicHUD>(It->Get()->GetHUD()) : nullptr)
		{
			if (UActivatableWidget* Widget = CreateWidget<UActivatableWidget>(It->Get(), WidgetClass))
			{
				HUD->AddWidgetToLayer(WidgetLayer, Widget);
			}
		}
	}
}

void UMalogicUISubsystem::DeliverWidgetInstanceToAllPlayers(EWidgetLayer WidgetLayer, UActivatableWidget* Widget)
{
	if (!Widget || !GetWorld())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AMalogicHUD* HUD = It->Get() ? Cast<AMalogicHUD>(It->Get()->GetHUD()) : nullptr)
		{
			HUD->AddWidgetToLayer(WidgetLayer, Widget);
		}
	}
}
