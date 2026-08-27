#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MalogicHUD.generated.h"

class UActivatableWidget;
class UPrimaryGameLayout;
enum class EWidgetLayer : uint8;

UCLASS()
class MALOGIC_API AMalogicHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool AddWidgetToLayer(EWidgetLayer WidgetLayer, UActivatableWidget* NewWidget);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool RemoveTopWidgetFrom(EWidgetLayer WidgetLayer);

	UPROPERTY(EditDefaultsOnly, Category = "Malogic|UI")
	TSubclassOf<UPrimaryGameLayout> PrimaryGameLayoutClass;

	UPrimaryGameLayout* GetPrimaryGameLayout() const { return PrimaryGameLayoutInstance; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UPrimaryGameLayout> PrimaryGameLayoutInstance;
};
