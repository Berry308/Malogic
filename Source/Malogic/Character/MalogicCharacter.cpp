// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/MalogicCharacter.h"

#include "AbilitySystem/MalogicAbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/MalogicCharacterMovementComp.h"
#include "Character/MalogicHealthComponent.h"
#include "Character/MalogicPawnExtensionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameModes/MalogicGameMode.h"
#include "MalogicGameplayTags.h"
#include "Player/MalogicPlayerController.h"
#include "Player/MalogicPlayerState.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MalogicCharacter)

//用于设置角色胶囊体和网格体的碰撞配置文件名称，可在DefaultEngine.ini中定义
namespace MalogicCharacter
{
	const FName CapsuleCollisionProfile(TEXT("MalogicPawnCapsule"));
	const FName MeshCollisionProfile(TEXT("MalogicPawnMesh"));
}

AMalogicCharacter::AMalogicCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMalogicCharacterMovementComp>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	SetNetCullDistanceSquared(900000000.0f);

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);
	//CapsuleComp->SetCollisionProfileName(MalogicCharacter::CapsuleCollisionProfile);

	USkeletalMeshComponent* MeshComponent = GetMesh();
	check(MeshComponent);
	MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	//MeshComponent->SetCollisionProfileName(MalogicCharacter::MeshCollisionProfile);
	MeshComponent->SetOwnerNoSee(true);
	MeshComponent->CastShadow = true;
	MeshComponent->bCastHiddenShadow = true;
	MeshComponent->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	UMalogicCharacterMovementComp* MovementComponent = CastChecked<UMalogicCharacterMovementComp>(GetCharacterMovement());
	MovementComponent->GravityScale = 1.0f;
	MovementComponent->MaxAcceleration = 2400.0f;
	MovementComponent->BrakingFrictionFactor = 1.0f;
	MovementComponent->BrakingFriction = 6.0f;
	MovementComponent->GroundFriction = 8.0f;
	MovementComponent->BrakingDecelerationWalking = 1400.0f;
	MovementComponent->bUseControllerDesiredRotation = false;
	MovementComponent->bOrientRotationToMovement = false;
	MovementComponent->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	MovementComponent->bAllowPhysicsRotationDuringAnimRootMotion = false;
	MovementComponent->GetNavAgentPropertiesRef().bCanCrouch = true;
	MovementComponent->bCanWalkOffLedgesWhenCrouching = true;
	MovementComponent->SetCrouchedHalfHeight(65.0f);

	PawnExtensionComponent = CreateDefaultSubobject<UMalogicPawnExtensionComponent>(TEXT("PawnExtensionComponent"));
	PawnExtensionComponent->OnAbilitySystemInitialized_RegisterAndCall(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemInitialized));
	PawnExtensionComponent->OnAbilitySystemUninitialized_Register(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnAbilitySystemUninitialized));

	HealthComponent = CreateDefaultSubobject<UMalogicHealthComponent>(TEXT("HealthComponent"));
	HealthComponent->OnDeathStarted.AddDynamic(this, &ThisClass::OnDeathStarted);
	HealthComponent->OnDeathFinished.AddDynamic(this, &ThisClass::OnDeathFinished);

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(CapsuleComp);
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->SetCastShadow(false);
	FirstPersonMesh->bCastDynamicShadow = false;
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(TEXT("NoCollision"));

	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, TEXT("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 90.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	BaseEyeHeight = 80.0f;
	CrouchedEyeHeight = 50.0f;
}

void AMalogicCharacter::Reset()
{
	DisableMovementAndCollision();
	K2_OnReset();
	UninitAndDestroy();
}

void AMalogicCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
}

AMalogicPlayerController* AMalogicCharacter::GetMalogicPlayerController() const
{
	return CastChecked<AMalogicPlayerController>(GetController(), ECastCheckedType::NullAllowed);
}

AMalogicPlayerState* AMalogicCharacter::GetMalogicPlayerState() const
{
	return CastChecked<AMalogicPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

UMalogicAbilitySystemComponent* AMalogicCharacter::GetMalogicAbilitySystemComponent() const
{
	return PawnExtensionComponent ? PawnExtensionComponent->GetMalogicAbilitySystemComponent() : nullptr;
}

UAbilitySystemComponent* AMalogicCharacter::GetAbilitySystemComponent() const
{
	return GetMalogicAbilitySystemComponent();
}

void AMalogicCharacter::OnAbilitySystemInitialized()
{
	UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent();
	check(AbilitySystemComponent);
	HealthComponent->InitializeWithAbilitySystem(AbilitySystemComponent);
	InitializeGameplayTags();
}

void AMalogicCharacter::OnAbilitySystemUninitialized()
{
	HealthComponent->UninitializeFromAbilitySystem();
}

void AMalogicCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	PawnExtensionComponent->HandleControllerChanged();
}

void AMalogicCharacter::UnPossessed()
{
	Super::UnPossessed();
	PawnExtensionComponent->HandleControllerChanged();
}

void AMalogicCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	PawnExtensionComponent->HandleControllerChanged();
}

void AMalogicCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	PawnExtensionComponent->HandlePlayerStateReplicated();
}

void AMalogicCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PawnExtensionComponent->SetupPlayerInputComponent();
}

void AMalogicCharacter::InitializeGameplayTags()
{
	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		for (const TPair<uint8, FGameplayTag>& TagMapping : MalogicGameplayTags::MovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				AbilitySystemComponent->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		for (const TPair<uint8, FGameplayTag>& TagMapping : MalogicGameplayTags::CustomMovementModeTagMap)
		{
			if (TagMapping.Value.IsValid())
			{
				AbilitySystemComponent->SetLooseGameplayTagCount(TagMapping.Value, 0);
			}
		}

		const UMalogicCharacterMovementComp* MovementComponent = CastChecked<UMalogicCharacterMovementComp>(GetCharacterMovement());
		SetMovementModeTag(MovementComponent->MovementMode, MovementComponent->CustomMovementMode, true);
	}
}

void AMalogicCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	if (const UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		AbilitySystemComponent->GetOwnedGameplayTags(TagContainer);
	}
}

bool AMalogicCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return GetMalogicAbilitySystemComponent() && GetMalogicAbilitySystemComponent()->HasMatchingGameplayTag(TagToCheck);
}

bool AMalogicCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return GetMalogicAbilitySystemComponent() && GetMalogicAbilitySystemComponent()->HasAllMatchingGameplayTags(TagContainer);
}

bool AMalogicCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return GetMalogicAbilitySystemComponent() && GetMalogicAbilitySystemComponent()->HasAnyMatchingGameplayTags(TagContainer);
}

void AMalogicCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	(void)DamageType;
	HealthComponent->DamageSelfDestruct(true);
}

void AMalogicCharacter::OnDeathStarted(AActor* OwningActor)
{
	(void)OwningActor;
	DisableMovementAndCollision();
}

void AMalogicCharacter::OnDeathFinished(AActor* OwningActor)
{
	(void)OwningActor;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DestroyDueToDeath);
}

void AMalogicCharacter::DisableMovementAndCollision()
{
	if (GetController())
	{
		GetController()->SetIgnoreMoveInput(true);
	}

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CapsuleComp->SetCollisionResponseToAllChannels(ECR_Ignore);

	UMalogicCharacterMovementComp* MovementComponent = CastChecked<UMalogicCharacterMovementComp>(GetCharacterMovement());
	MovementComponent->StopMovementImmediately();
	MovementComponent->DisableMovement();
}

void AMalogicCharacter::DestroyDueToDeath()
{
	K2_OnDeathFinished();
	UninitAndDestroy();
}

void AMalogicCharacter::UninitAndDestroy()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		DetachFromControllerPendingDestroy();

		//AController* PreviousController = GetController();
		//if (AMalogicGameMode* GameMode = GetWorld()->GetAuthGameMode<AMalogicGameMode>())
		//{
		//	GameMode->RequestPlayerRestartNextFrame(PreviousController);
		//}

		SetLifeSpan(0.1f);
	}

	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		if (AbilitySystemComponent->GetAvatarActor() == this)
		{
			PawnExtensionComponent->UninitializeAbilitySystem();
		}
	}

	SetActorHiddenInGame(true);
}

void AMalogicCharacter::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	const UMalogicCharacterMovementComp* MovementComponent = CastChecked<UMalogicCharacterMovementComp>(GetCharacterMovement());
	SetMovementModeTag(PreviousMovementMode, PreviousCustomMode, false);
	SetMovementModeTag(MovementComponent->MovementMode, MovementComponent->CustomMovementMode, true);
}

void AMalogicCharacter::SetMovementModeTag(EMovementMode MovementMode, uint8 CustomMovementMode, bool bTagEnabled)
{
	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		const FGameplayTag* MovementModeTag = MovementMode == MOVE_Custom
			? MalogicGameplayTags::CustomMovementModeTagMap.Find(CustomMovementMode)
			: MalogicGameplayTags::MovementModeTagMap.Find(MovementMode);

		if (MovementModeTag && MovementModeTag->IsValid())
		{
			AbilitySystemComponent->SetLooseGameplayTagCount(*MovementModeTag, bTagEnabled ? 1 : 0);
		}
	}
}

void AMalogicCharacter::ToggleCrouch()
{
	const UMalogicCharacterMovementComp* MovementComponent = CastChecked<UMalogicCharacterMovementComp>(GetCharacterMovement());
	if (IsCrouched() || MovementComponent->bWantsToCrouch)
	{
		UnCrouch();
	}
	else if (MovementComponent->IsMovingOnGround())
	{
		Crouch();
	}
}

void AMalogicCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Crouching, 1);
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AMalogicCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	if (UMalogicAbilitySystemComponent* AbilitySystemComponent = GetMalogicAbilitySystemComponent())
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(MalogicGameplayTags::Status_Crouching, 0);
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

bool AMalogicCharacter::CanJumpInternal_Implementation() const
{
	return JumpIsAllowedInternal();
}
