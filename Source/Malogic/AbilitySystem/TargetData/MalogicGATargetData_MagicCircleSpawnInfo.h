#pragma once

#include "Abilities/GameplayAbilityTargetTypes.h"

#include "MalogicGATargetData_MagicCircleSpawnInfo.generated.h"

/** Target data containing the source and complete deployment transforms for a magic circle. */
USTRUCT()
struct MALOGIC_API FMalogicGATargetData_MagicCircleSpawnInfo : public FGameplayAbilityTargetData_LocationInfo
{
	GENERATED_BODY()

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FMalogicGATargetData_MagicCircleSpawnInfo::StaticStruct();
	}

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
	{
		return FGameplayAbilityTargetData_LocationInfo::NetSerialize(Ar, Map, bOutSuccess);
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
