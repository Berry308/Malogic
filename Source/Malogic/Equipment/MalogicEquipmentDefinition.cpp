// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicEquipmentDefinition.h"
#include "MalogicEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicEquipmentDefinition)

UMalogicEquipmentDefinition::UMalogicEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstanceType = UMalogicEquipmentInstance::StaticClass();
}
