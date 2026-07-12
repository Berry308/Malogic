// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicGameplayEffectContext.h"

#include "AbilitySystem/MalogicAbilitySourceInterface.h"
#include "Engine/HitResult.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#if UE_WITH_IRIS
//#include "Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h"
//#include "Serialization/GameplayEffectContextNetSerializer.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicGameplayEffectContext)

class FArchive;

FMalogicGameplayEffectContext* FMalogicGameplayEffectContext::ExtractEffectContext(struct FGameplayEffectContextHandle Handle)
{
	FGameplayEffectContext* BaseEffectContext = Handle.Get();
	if ((BaseEffectContext != nullptr) && BaseEffectContext->GetScriptStruct()->IsChildOf(FMalogicGameplayEffectContext::StaticStruct()))
	{
		return (FMalogicGameplayEffectContext*)BaseEffectContext;
	}

	return nullptr;
}

bool FMalogicGameplayEffectContext::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess);

	// Not serialized for post-activation use:
	// CartridgeID

	return true;
}

#if UE_WITH_IRIS
//namespace UE::Net
//{
//	// Forward to FGameplayEffectContextNetSerializer
//	// Note: If FMalogicGameplayEffectContext::NetSerialize() is modified, a custom NetSerializesr must be implemented as the current fallback will no longer be sufficient.
//	UE_NET_IMPLEMENT_FORWARDING_NETSERIALIZER_AND_REGISTRY_DELEGATES(MRGameplayEffectContext, FGameplayEffectContextNetSerializer);
//}
#endif

void FMalogicGameplayEffectContext::SetAbilitySource(const IMalogicAbilitySourceInterface* InObject, float InSourceLevel)
{
	AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
	//SourceLevel = InSourceLevel;
}

const IMalogicAbilitySourceInterface* FMalogicGameplayEffectContext::GetAbilitySource() const
{
	return Cast<IMalogicAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FMalogicGameplayEffectContext::GetPhysicalMaterial() const
{
	if (const FHitResult* HitResultPtr = GetHitResult())
	{
		return HitResultPtr->PhysMaterial.Get();
	}
	return nullptr;
}

