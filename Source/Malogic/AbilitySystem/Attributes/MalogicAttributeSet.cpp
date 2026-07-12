// Fill out your copyright notice in the Description page of Project Settings.


#include "MalogicAttributeSet.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicAttributeSet)

class UWorld;


UMalogicAttributeSet::UMalogicAttributeSet()
{
}

UWorld* UMalogicAttributeSet::GetWorld() const
{
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UMalogicAbilitySystemComponent* UMalogicAttributeSet::GetMalogicAbilitySystemComponent() const
{
	return Cast<UMalogicAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}
