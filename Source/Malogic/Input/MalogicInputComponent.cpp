// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicInputComponent.h"

#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicInputComponent)

class UMalogicInputConfig;

UMalogicInputComponent::UMalogicInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UMalogicInputComponent::AddInputMappings(const UMalogicInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to add something from your input config if required
}

void UMalogicInputComponent::RemoveInputMappings(const UMalogicInputConfig* InputConfig, UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// Here you can handle any custom logic to remove input mappings that you may have added above
}

void UMalogicInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}
