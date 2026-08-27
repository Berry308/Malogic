#pragma once

#include "CoreMinimal.h"
#include "Components/Overlay.h"
#include "UI/ActivatableWidget.h"
#include "ActivatableWidgetStack.generated.h"

UCLASS()
class MALOGIC_API UActivatableWidgetStack : public UOverlay
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Malogic|UI")
	UActivatableWidget* GetTopWidget() const { return WidgetStack.Num() > 0 ? WidgetStack.Last() : nullptr; }

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PushWidget(UActivatableWidget* NewWidget);

	UFUNCTION(BlueprintCallable, Category = "Malogic|UI")
	bool PopWidget();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UActivatableWidget>> WidgetStack;
};
