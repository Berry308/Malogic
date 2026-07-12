// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"

#include "MalogicGameplayEffectContext.generated.h"

class AActor;
class FArchive;
class IMalogicAbilitySourceInterface;
class UObject;
class UPhysicalMaterial;

//或许需要在项目内配置?
USTRUCT()
struct FMalogicGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FMalogicGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FMalogicGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}

	/** Returns the wrapped FMalogicGameplayEffectContext from the handle, or nullptr if it doesn't exist or is the wrong type */
	static FMalogicGameplayEffectContext* ExtractEffectContext(struct FGameplayEffectContextHandle Handle);

	/** Sets the object used as the ability source */
	void SetAbilitySource(const IMalogicAbilitySourceInterface* InObject, float InSourceLevel);

	/** Returns the ability source interface associated with the source object. Only valid on the authority. */
	const IMalogicAbilitySourceInterface* GetAbilitySource() const;

	//返回带有当前HitResult的FMRGameplayEffectContext副本
	virtual FGameplayEffectContext* Duplicate() const override
	{
		FMalogicGameplayEffectContext* NewContext = new FMalogicGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FMalogicGameplayEffectContext::StaticStruct();
	}

	/** Overridden to serialize new fields */
	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

	/** Returns the physical material from the hit result if there is one */
	const UPhysicalMaterial* GetPhysicalMaterial() const;

public:
	/** ID to allow the identification of multiple bullets that were part of the same cartridge */
	UPROPERTY()
	int32 CartridgeID = -1;

protected:
	/** Ability Source object (should implement IMalogicAbilitySourceInterface). NOT replicated currently */
	UPROPERTY()
	TWeakObjectPtr<const UObject> AbilitySourceObject;
};

template<>
struct TStructOpsTypeTraits<FMalogicGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FMalogicGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

