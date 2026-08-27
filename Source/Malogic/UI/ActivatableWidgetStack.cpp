#include "UI/ActivatableWidgetStack.h"

#include "Components/OverlaySlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ActivatableWidgetStack)

bool UActivatableWidgetStack::PushWidget(UActivatableWidget* NewWidget)
{
	if (!IsValid(NewWidget) || WidgetStack.Contains(NewWidget) || NewWidget->GetParent())
	{
		return false;
	}

	if (UActivatableWidget* PreviousWidget = GetTopWidget())
	{
		PreviousWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UOverlaySlot* OverlaySlot = AddChildToOverlay(NewWidget))
	{
		OverlaySlot->SetHorizontalAlignment(NewWidget->HorizontalAlignment);
		OverlaySlot->SetVerticalAlignment(NewWidget->VerticalAlignment);
	}

	WidgetStack.Add(NewWidget);
	NewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	return true;
}

bool UActivatableWidgetStack::PopWidget()
{
	if (WidgetStack.Num() == 0)
	{
		return false;
	}

	if (UActivatableWidget* Widget = WidgetStack.Pop())
	{
		Widget->RemoveFromParent();
	}

	if (UActivatableWidget* PreviousWidget = GetTopWidget())
	{
		PreviousWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	return true;
}
