#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ActivatableWidget.generated.h"

UENUM(BlueprintType)
enum class EWidgetInputMode : uint8
{
	GameAndUI,
	UIOnly,
	GameOnly
};

USTRUCT(BlueprintType)
struct FWidgetInputModeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EWidgetInputMode InputMode = EWidgetInputMode::GameAndUI;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bLockMouseToViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bShowMouseCursor = false;
};

UCLASS()
class MALOGIC_API UActivatableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Malogic|UI")
	FWidgetInputModeConfig GetInputModeConfig() const { return InputModeConfig; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment = HAlign_Fill;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment = VAlign_Fill;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	FWidgetInputModeConfig InputModeConfig;
};
