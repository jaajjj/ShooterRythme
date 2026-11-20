// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/CryptZombie.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

ACryptZombie::ACryptZombie()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		MoveComp->MaxWalkSpeed = 350.0f;
	}
}

void ACryptZombie::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	CachedAIController = Cast<AAIController>(GetController());

	if (GetWorld() && ChaseInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ChaseTimerHandle,
			this,
			&ACryptZombie::UpdateChase,
			ChaseInterval,
			true
		);
	}
}

void ACryptZombie::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HitDelayTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void ACryptZombie::UpdateChase()
{
	if (bIsDead || bIsAttacking)
	{
		return;
	}

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerChar)
	{
		return;
	}

	const float DistanceToPlayer = FVector::Dist(GetActorLocation(), PlayerChar->GetActorLocation());

	if (DistanceToPlayer <= AttackRange)
	{
		PerformAttack(PlayerChar);
	}
	else
	{
		if (!CachedAIController)
		{
			CachedAIController = Cast<AAIController>(GetController());
		}

		if (CachedAIController)
		{
			CachedAIController->MoveToActor(PlayerChar, AttackRange * 0.7f);
		}
	}
}

void ACryptZombie::PerformAttack(AActor* Target)
{
	if (bIsDead || bIsAttacking || !Target)
	{
		return;
	}

	bIsAttacking = true;
	CurrentAttackTarget = Target;

	if (CachedAIController)
	{
		CachedAIController->StopMovement();
	}

	if (AttackMontage)
	{
		PlayAnimMontage(AttackMontage);
	}

	if (AttackHitDelay > 0.0f)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				HitDelayTimerHandle,
				this,
				&ACryptZombie::DealDamage,
				AttackHitDelay,
				false
			);
		}
	}
	else
	{
		DealDamage();
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AttackCooldownTimerHandle,
			this,
			&ACryptZombie::ResetAttack,
			AttackCooldown,
			false
		);
	}
}

void ACryptZombie::DealDamage()
{
	if (bIsDead || !CurrentAttackTarget)
	{
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), CurrentAttackTarget->GetActorLocation());
	if (Distance <= AttackRange * 1.4f)
	{
		UGameplayStatics::ApplyDamage(
			CurrentAttackTarget,
			AttackDamage,
			GetController(),
			this,
			UDamageType::StaticClass()
		);

		OnZombieAttack(CurrentAttackTarget);
	}

	CurrentAttackTarget = nullptr;
}

void ACryptZombie::ResetAttack()
{
	if (!bIsDead)
	{
		bIsAttacking = false;
	}
}

float ACryptZombie::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);

	if (HitMontage && CurrentHealth > 0.0f)
	{
		PlayAnimMontage(HitMontage);
	}

	OnZombieTakeDamage(ActualDamage, DamageCauser);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void ACryptZombie::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bIsAttacking = false;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HitDelayTimerHandle);
	}
	CurrentAttackTarget = nullptr;

	if (CachedAIController)
	{
		CachedAIController->StopMovement();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
		MoveComp->StopMovementImmediately();
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}

	OnDeathDelegate.Broadcast();
	OnZombieDeath();

	SetLifeSpan(DeathLifeSpan);
}
