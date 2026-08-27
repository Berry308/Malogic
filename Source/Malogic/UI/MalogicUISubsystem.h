#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MalogicUISubsystem.generated.h"

class UActivatableWidget;
class UPrimaryGameLayout;
enum class EWidgetLayer : uint8;

UCLASS()
class MALOGIC_API UMalogicUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	void DeliverWidgetToAllPlayers(EWidgetLayer WidgetLayer, TSubclassOf<UActivatableWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	void DeliverWidgetInstanceToAllPlayers(EWidgetLayer WidgetLayer, UActivatableWidget* Widget);
};
