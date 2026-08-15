#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "MalogicGATargetData_MagicCircleSpawnInfo.generated.h"

/** Target data containing the source and complete deployment transforms for a magic circle. */
USTRUCT()
struct MALOGIC_API FMalogicGATargetData_MagicCircleSpawnInfo : public FGameplayAbilityTargetData_LocationInfo
{
	GENERATED_BODY()

	/** Server-synchronized timestamp captured when the client created this deployment request. */
	UPROPERTY()
	float ClientSpawnTime = 0.0f;

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FMalogicGATargetData_MagicCircleSpawnInfo::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		const bool bParentSuccess = FGameplayAbilityTargetData_LocationInfo::NetSerialize(Ar, Map, bOutSuccess);
		Ar << ClientSpawnTime;
		bOutSuccess &= bParentSuccess;
		return bParentSuccess;
	}
};

template<>
struct TStructOpsTypeTraits<FMalogicGATargetData_MagicCircleSpawnInfo> : public TStructOpsTypeTraitsBase2<FMalogicGATargetData_MagicCircleSpawnInfo>
{
	enum
	{
		WithNetSerializer = true
	};
};
