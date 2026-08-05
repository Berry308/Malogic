// Copyright Epic Games, Inc. All Rights Reserved.

#include "MalogicEquipmentInstance.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Character/MalogicCharacter.h"
#include "MalogicEquipmentDefinition.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicEquipmentInstance)

class FLifetimeProperty;
class UClass;
class USceneComponent;

UMalogicEquipmentInstance::UMalogicEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UWorld* UMalogicEquipmentInstance::GetWorld() const
{
	if (APawn* OwningPawn = GetPawn())
	{
		return OwningPawn->GetWorld();
	}
	return nullptr;
}

void UMalogicEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Instigator);
	DOREPLIFETIME(ThisClass, SpawnedActors);
}

APawn* UMalogicEquipmentInstance::GetPawn() const
{
	return Cast<APawn>(GetOuter());
}

APawn* UMalogicEquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}
	return Result;
}

void UMalogicEquipmentInstance::SpawnEquipmentActors(const TArray<FMalogicEquipmentActorToSpawn>& ActorsToSpawn)
{
	if (APawn* OwningPawn = GetPawn())
	{
		USceneComponent* AttachTarget = OwningPawn->GetRootComponent();
		if (ACharacter* Char = Cast<ACharacter>(OwningPawn))
		{
			AttachTarget = Char->GetMesh();
		}

		USceneComponent* AttachTargetFirstPerson = OwningPawn->GetRootComponent();
		if (AMalogicCharacter* Char = Cast<AMalogicCharacter>(OwningPawn))
		{
			AttachTargetFirstPerson = Char->GetFirstPersonMesh();
		}

		for (const FMalogicEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
		{
			AActor* ThirdPersonNewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
			ThirdPersonNewActor->FinishSpawning(FTransform::Identity, true);
			if (USkeletalMeshComponent* MeshComp = ThirdPersonNewActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				MeshComp->SetOwnerNoSee(true);
			}
			ThirdPersonNewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
			ThirdPersonNewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);

			if (AttachTargetFirstPerson)
			{
				AActor* FirstPersonNewActor = GetWorld()->SpawnActorDeferred<AActor>(SpawnInfo.ActorToSpawn, FTransform::Identity, OwningPawn);
				FirstPersonNewActor->FinishSpawning(FTransform::Identity, true);
				if (USkeletalMeshComponent* MeshComp = FirstPersonNewActor->FindComponentByClass<USkeletalMeshComponent>())
				{
					MeshComp->SetOnlyOwnerSee(true);
				}
				FirstPersonNewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
				FirstPersonNewActor->AttachToComponent(AttachTargetFirstPerson, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);
				SpawnedActors.Add(FirstPersonNewActor);
			}

			SpawnedActors.Add(ThirdPersonNewActor);
		}
	}
}

void UMalogicEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor)
		{
			Actor->Destroy();
		}
	}
}

void UMalogicEquipmentInstance::OnEquipped()
{
	K2_OnEquipped();
}

void UMalogicEquipmentInstance::OnUnequipped()
{
	K2_OnUnequipped();
}

void UMalogicEquipmentInstance::OnRep_Instigator()
{
}

//TSubclassOf<UAnimInstance> UMalogicEquipmentInstance::GetFirstPersonAnimInstanceClass() const
//{
//	return FirstPersonAnimInstanceClass;
//}

TSubclassOf<UAnimInstance> UMalogicEquipmentInstance::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}
