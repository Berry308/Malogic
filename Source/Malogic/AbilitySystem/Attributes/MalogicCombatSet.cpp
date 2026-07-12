
#include "AbilitySystem/Attributes/MalogicCombatSet.h"

#include "MalogicAttributeSet.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicCombatSet)

class FLifetimeProperty;


UMalogicCombatSet::UMalogicCombatSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UMalogicCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UMalogicCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UMalogicCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UMalogicCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMalogicCombatSet, BaseDamage, OldValue);
}

void UMalogicCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMalogicCombatSet, BaseHeal, OldValue);
}

