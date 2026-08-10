#include "Weapon/MalogicMagicWeaponInstance.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Inventory/InventoryFragment_InputMapping.h"
#include "Inventory/MalogicInventoryItemInstance.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicMagicWeaponInstance)

void UMalogicMagicWeaponInstance::OnEquipped()
{
	Super::OnEquipped();

	bIsEquipped = true;
	ApplyInputMapping();
}

void UMalogicMagicWeaponInstance::OnUnequipped()
{
	bIsEquipped = false;
	RemoveInputMapping();

	Super::OnUnequipped();
}

void UMalogicMagicWeaponInstance::OnInstigatorChanged()
{
	if (bIsEquipped)
	{
		RemoveInputMapping();
		ApplyInputMapping();
	}
}

void UMalogicMagicWeaponInstance::ApplyInputMapping()
{
	if (bHasAppliedInputMapping)
	{
		return;
	}

	APawn* Pawn = GetPawn();
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	const UMalogicInventoryItemInstance* InventoryItem = Cast<UMalogicInventoryItemInstance>(GetInstigator());
	const UInventoryFragment_InputMapping* InputMappingFragment = InventoryItem ? InventoryItem->FindFragmentByClass<UInventoryFragment_InputMapping>() : nullptr;
	if (!InputMappingFragment || InputMappingFragment->InputMappingContext.InputMapping.IsNull())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputMappingContext* MappingContext = InputMappingFragment->InputMappingContext.InputMapping.LoadSynchronous();
	if (!InputSubsystem || !MappingContext)
	{
		return;
	}

	if (InputMappingFragment->InputMappingContext.bRegisterWithSettings)
	{
		if (UEnhancedInputUserSettings* Settings = InputSubsystem->GetUserSettings())
		{
			Settings->RegisterInputMappingContext(MappingContext);
		}
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = false;
	InputSubsystem->AddMappingContext(MappingContext, InputMappingFragment->InputMappingContext.Priority, Options);

	AppliedInputMapping = InputMappingFragment->InputMappingContext;
	AppliedInputSubsystem = InputSubsystem;
	AppliedInputMappingContext = MappingContext;
	bHasAppliedInputMapping = true;
}

void UMalogicMagicWeaponInstance::RemoveInputMapping()
{
	if (!bHasAppliedInputMapping)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = AppliedInputSubsystem.Get();
	UInputMappingContext* MappingContext = AppliedInputMappingContext.Get();
	if (!MappingContext)
	{
		MappingContext = AppliedInputMapping.InputMapping.LoadSynchronous();
	}

	if (InputSubsystem && MappingContext)
	{
		InputSubsystem->RemoveMappingContext(MappingContext);
	}

	AppliedInputMapping = FInputMappingContextAndPriority();
	AppliedInputSubsystem = nullptr;
	AppliedInputMappingContext = nullptr;
	bHasAppliedInputMapping = false;
}
