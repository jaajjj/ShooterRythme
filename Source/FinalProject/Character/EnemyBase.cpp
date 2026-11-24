// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/EnemyBase.h"
#include "Character/FinalProjectCharacter.h"
#include "AIController.h"
#include "DetourCrowdAIController.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Animation/AnimInstance.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

AEnemyBase::AEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// Utilise le contrôleur IA Detour Crowd pour l'évitement automatique de foule
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = ADetourCrowdAIController::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
		MoveComp->MaxWalkSpeed = 350.0f;

		// --- Évitement de foule (RVO Avoidance) ---
		MoveComp->bUseRVOAvoidance = true;
		MoveComp->AvoidanceConsiderationRadius = 250.0f;
		MoveComp->AvoidanceWeight = 0.5f;
		MoveComp->SetAvoidanceGroup(1);
		MoveComp->SetGroupsToAvoid(1);

		// Poussée physique douce entre personnages pour fluidifier les contacts
		MoveComp->bEnablePhysicsInteraction = true;
		MoveComp->PushForceFactor = 75000.0f;
		MoveComp->InitialPushForceFactor = 50000.0f;
		MoveComp->bPushForceScaledToMass = true;
	}
}

void AEnemyBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (EnemyData)
	{
		ApplyEnemyData(EnemyData);
	}
}

void AEnemyBase::ApplyEnemyData(UEnemyDataAsset* InData)
{
	if (!InData)
	{
		return;
	}

	EnemyData = InData;

	MaxHealth = InData->MaxHealth;
	CurrentHealth = MaxHealth;

	AttackDamage = InData->AttackDamage;
	AttackRange = InData->AttackRange;
	AttackCooldown = InData->AttackCooldown;
	AttackHitDelay = InData->AttackHitDelay;
	DeathLifeSpan = InData->DeathLifeSpan;

	AttackMontage = InData->AttackMontage;
	AttackMontages = InData->AttackMontages;
	DeathMontage = InData->DeathMontage;
	HitMontage = InData->HitMontage;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = InData->MovementSpeed;
	}

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (InData->SkeletalMesh)
		{
			MeshComp->SetSkeletalMeshAsset(InData->SkeletalMesh);
		}

		if (InData->AnimClass)
		{
			MeshComp->SetAnimInstanceClass(InData->AnimClass);
		}

		MeshComp->SetRelativeLocation(InData->MeshRelativeLocation);
		MeshComp->SetRelativeRotation(InData->MeshRelativeRotation);
		MeshComp->SetRelativeScale3D(InData->MeshRelativeScale);
	}
}

void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	if (EnemyData)
	{
		ApplyEnemyData(EnemyData);
	}

	CurrentHealth = MaxHealth;
	CachedAIController = Cast<AAIController>(GetController());

	if (ADetourCrowdAIController* CrowdAI = Cast<ADetourCrowdAIController>(CachedAIController))
	{
		if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(CrowdAI->GetPathFollowingComponent()))
		{
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
			CrowdComp->SetAvoidanceGroup(1);
			CrowdComp->SetGroupsToAvoid(1);
			CrowdComp->SetCrowdSeparation(true);
			CrowdComp->SetCrowdSeparationWeight(2.0f);
		}
	}

	if (GetWorld() && ChaseInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ChaseTimerHandle,
			this,
			&AEnemyBase::UpdateChase,
			ChaseInterval,
			true
		);
	}
}

void AEnemyBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HitDelayTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReturnToPoolTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AEnemyBase::UpdateChase()
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
			CachedAIController->MoveToActor(PlayerChar, AttackRange * 0.7f, true, true, true);
		}
	}
}

void AEnemyBase::PerformAttack(AActor* Target)
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

	UAnimMontage* SelectedMontage = GetRandomAttackMontage();
	if (SelectedMontage)
	{
		PlayAnimMontage(SelectedMontage);
	}

	if (AttackHitDelay > 0.0f)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				HitDelayTimerHandle,
				this,
				&AEnemyBase::DealDamage,
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
			&AEnemyBase::ResetAttack,
			AttackCooldown,
			false
		);
	}
}

UAnimMontage* AEnemyBase::GetRandomAttackMontage() const
{
	// S'il y a des montages configurés dans la liste AttackMontages
	if (AttackMontages.Num() > 0)
	{
		// Filtrer les entrées valides non nulles
		TArray<UAnimMontage*> ValidMontages;
		for (UAnimMontage* Montage : AttackMontages)
		{
			if (Montage)
			{
				ValidMontages.Add(Montage);
			}
		}

		if (ValidMontages.Num() > 0)
		{
			const int32 RandomIndex = FMath::RandRange(0, ValidMontages.Num() - 1);
			return ValidMontages[RandomIndex];
		}
	}

	// Sinon, retourner le montage unique par défaut s'il existe
	return AttackMontage;
}

void AEnemyBase::DealDamage()
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

		OnEnemyAttack(CurrentAttackTarget);
	}

	CurrentAttackTarget = nullptr;
}

void AEnemyBase::ResetAttack()
{
	if (!bIsDead)
	{
		bIsAttacking = false;
	}
}

float AEnemyBase::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
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

	OnEnemyTakeDamage(ActualDamage, DamageCauser);

	if (CurrentHealth <= 0.0f)
	{
		Die();
	}

	return ActualDamage;
}

void AEnemyBase::Die()
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

	// Incrémente le compteur de monstres tués du joueur (+1)
	if (AFinalProjectCharacter* PlayerChar = Cast<AFinalProjectCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0)))
	{
		PlayerChar->AddKill();
	}

	OnDeathDelegate.Broadcast(this);
	OnEnemyDeath();

	if (bIsPooled)
	{
		// Attend la fin de l'anim de mort puis désactive pour retourner dans le pool
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				ReturnToPoolTimerHandle,
				this,
				&AEnemyBase::DeactivateToPool,
				FMath::Max(1.5f, DeathLifeSpan),
				false
			);
		}
	}
	else
	{
		SetLifeSpan(DeathLifeSpan);
	}
}

void AEnemyBase::ActivateFromPool(UEnemyDataAsset* InData, const FTransform& SpawnTransform)
{
	bIsPooled = true;
	bIsDead = false;
	bIsAttacking = false;
	CurrentAttackTarget = nullptr;

	// Réinitialise le LifeSpan pour s'assurer qu'Unreal ne détruise jamais cet acteur recyclé
	SetLifeSpan(0.0f);

	// Annuler tout timer résiduel (notamment l'ancien timer de retour au pool !)
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HitDelayTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReturnToPoolTimerHandle);
	}

	// Repositionner l'ennemi
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

	// Appliquer les données (mesh, anims, stats)
	if (InData)
	{
		ApplyEnemyData(InData);
	}

	CurrentHealth = MaxHealth;

	// Réactiver affichage et collisions
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// Réactiver la marche
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->StopMovementImmediately();
	}

	// Réactiver l'IA et relancer la traque avec Detour Crowd
	if (!CachedAIController)
	{
		CachedAIController = Cast<AAIController>(GetController());
	}
	if (!CachedAIController)
	{
		SpawnDefaultController();
		CachedAIController = Cast<AAIController>(GetController());
	}

	if (ADetourCrowdAIController* CrowdAI = Cast<ADetourCrowdAIController>(CachedAIController))
	{
		if (UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(CrowdAI->GetPathFollowingComponent()))
		{
			CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
			CrowdComp->SetAvoidanceGroup(1);
			CrowdComp->SetGroupsToAvoid(1);
			CrowdComp->SetCrowdSeparation(true);
			CrowdComp->SetCrowdSeparationWeight(2.0f);
		}
	}

	if (GetWorld() && ChaseInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ChaseTimerHandle,
			this,
			&AEnemyBase::UpdateChase,
			ChaseInterval,
			true
		);
	}
}

void AEnemyBase::DeactivateToPool()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ChaseTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AttackCooldownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(HitDelayTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReturnToPoolTimerHandle);
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

	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	// Notifier le WaveSpawner que cet ennemi est prêt à être réutilisé
	OnDeactivatedDelegate.Broadcast(this);
}
