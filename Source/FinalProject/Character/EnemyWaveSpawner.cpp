// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/EnemyWaveSpawner.h"
#include "Character/EnemyBase.h"
#include "Character/EnemyDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/DamageEvents.h"
#include "Engine/TargetPoint.h"

AEnemyWaveSpawner::AEnemyWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	EnemyBaseClass = AEnemyBase::StaticClass();

	// Configuration par défaut des monstres 
	FEnemyTierConfig GoblinTier;
	GoblinTier.EnemyName = TEXT("Gobelin");
	GoblinTier.UnlockTime = 0.0f; 
	GoblinTier.SpawnWeight = 1.0f;
	EnemyTiers.Add(GoblinTier);

	FEnemyTierConfig ZombieTier;
	ZombieTier.EnemyName = TEXT("Zombie");
	ZombieTier.UnlockTime = 60.0f; 
	ZombieTier.SpawnWeight = 0.8f;
	EnemyTiers.Add(ZombieTier);

	FEnemyTierConfig GolemTier;
	GolemTier.EnemyName = TEXT("Golem");
	GolemTier.UnlockTime = 120.0f; 
	GolemTier.SpawnWeight = 0.35f;
	EnemyTiers.Add(GolemTier);
}

void AEnemyWaveSpawner::BeginPlay()
{
	Super::BeginPlay();

	ElapsedGameTime = 0.0f;

	// Détection automatique de tous les TargetPoints placés dans la scène
	if (SpawnPoints.Num() == 0)
	{
		TArray<AActor*> FoundPoints;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATargetPoint::StaticClass(), FoundPoints);
		for (AActor* Pt : FoundPoints)
		{
			if (IsValid(Pt))
			{
				SpawnPoints.Add(Pt);
			}
		}
	}

	InitializePool();

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AEnemyWaveSpawner::TriggerSpawnCycle,
			InitialSpawnInterval,
			false
		);
	}
}

void AEnemyWaveSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedGameTime += DeltaTime;
	CurrentAliveCount = ActiveEnemies.Num();
	CurrentAllowedMaxAlive = GetCurrentTargetAliveCap();
}

void AEnemyWaveSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AEnemyWaveSpawner::InitializePool()
{
	UWorld* World = GetWorld();
	if (!World || !EnemyBaseClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector PoolStorageLoc = FVector(0.0f, 0.0f, -10000.0f);
	const FRotator PoolStorageRot = FRotator::ZeroRotator;

	for (int32 i = 0; i < InitialPoolSize; ++i)
	{
		AEnemyBase* NewEnemy = World->SpawnActor<AEnemyBase>(EnemyBaseClass, PoolStorageLoc, PoolStorageRot, SpawnParams);
		if (NewEnemy)
		{
			NewEnemy->bIsPooled = true;
			NewEnemy->OnDeathDelegate.AddDynamic(this, &AEnemyWaveSpawner::OnEnemyDied);
			NewEnemy->OnDeactivatedDelegate.AddDynamic(this, &AEnemyWaveSpawner::OnEnemyDeactivated);
			NewEnemy->DeactivateToPool();
		}
	}

	// Dégripper la collision entre ennemis : les ennemis s'ignorent physiquement lors de leurs déplacements
	// tout en bloquant fermement le joueur et le décor. L'évitement Detour Crowd et RVO gèrent leur espacement.
	for (int32 i = 0; i < InactivePool.Num(); ++i)
	{
		for (int32 j = i + 1; j < InactivePool.Num(); ++j)
		{
			if (InactivePool[i] && InactivePool[j])
			{
				InactivePool[i]->GetCapsuleComponent()->IgnoreActorWhenMoving(InactivePool[j], true);
				InactivePool[j]->GetCapsuleComponent()->IgnoreActorWhenMoving(InactivePool[i], true);
			}
		}
	}
}

AEnemyBase* AEnemyWaveSpawner::GetOrCreateEnemyFromPool()
{
	while (InactivePool.Num() > 0)
	{
		AEnemyBase* Candidate = InactivePool.Pop();
		if (IsValid(Candidate) && !ActiveEnemies.Contains(Candidate))
		{
			InactivePool.Remove(Candidate);
			return Candidate;
		}
	}

	UWorld* World = GetWorld();
	if (World && EnemyBaseClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AEnemyBase* NewEnemy = World->SpawnActor<AEnemyBase>(EnemyBaseClass, FVector(0.0f, 0.0f, -10000.0f), FRotator::ZeroRotator, SpawnParams);
		if (NewEnemy)
		{
			NewEnemy->bIsPooled = true;
			NewEnemy->OnDeathDelegate.AddDynamic(this, &AEnemyWaveSpawner::OnEnemyDied);
			NewEnemy->OnDeactivatedDelegate.AddDynamic(this, &AEnemyWaveSpawner::OnEnemyDeactivated);

			for (AEnemyBase* Existing : ActiveEnemies)
			{
				if (Existing)
				{
					NewEnemy->GetCapsuleComponent()->IgnoreActorWhenMoving(Existing, true);
					Existing->GetCapsuleComponent()->IgnoreActorWhenMoving(NewEnemy, true);
				}
			}
			for (AEnemyBase* Existing : InactivePool)
			{
				if (Existing)
				{
					NewEnemy->GetCapsuleComponent()->IgnoreActorWhenMoving(Existing, true);
					Existing->GetCapsuleComponent()->IgnoreActorWhenMoving(NewEnemy, true);
				}
			}

			return NewEnemy;
		}
	}

	return nullptr;
}

void AEnemyWaveSpawner::TriggerSpawnCycle()
{
	if (ActiveEnemies.Num() < GetCurrentTargetAliveCap())
	{
		SpawnSingleEnemy();
	}

	const float NextInterval = GetCurrentSpawnInterval();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AEnemyWaveSpawner::TriggerSpawnCycle,
			NextInterval,
			false
		);
	}
}

void AEnemyWaveSpawner::SpawnSingleEnemy()
{
	UEnemyDataAsset* ChosenData = ChooseEnemyDataByTime();
	if (!ChosenData)
	{
		return;
	}

	AEnemyBase* Enemy = GetOrCreateEnemyFromPool();
	if (!Enemy)
	{
		return;
	}

	const FTransform SpawnTransform = CalculateSpawnTransform();
	ActiveEnemies.Add(Enemy);
	Enemy->ActivateFromPool(ChosenData, SpawnTransform);
}

UEnemyDataAsset* AEnemyWaveSpawner::ChooseEnemyDataByTime() const
{
	TArray<const FEnemyTierConfig*> AvailableTiers;
	float TotalWeight = 0.0f;

	for (const FEnemyTierConfig& Tier : EnemyTiers)
	{
		if (Tier.EnemyData && ElapsedGameTime >= Tier.UnlockTime && Tier.SpawnWeight > 0.0f)
		{
			AvailableTiers.Add(&Tier);
			TotalWeight += Tier.SpawnWeight;
		}
	}

	if (AvailableTiers.Num() == 0)
	{
		for (const FEnemyTierConfig& Tier : EnemyTiers)
		{
			if (Tier.EnemyData)
			{
				return Tier.EnemyData;
			}
		}
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	for (const FEnemyTierConfig* Tier : AvailableTiers)
	{
		if (Roll <= Tier->SpawnWeight)
		{
			return Tier->EnemyData;
		}
		Roll -= Tier->SpawnWeight;
	}

	return AvailableTiers.Last()->EnemyData;
}

FTransform AEnemyWaveSpawner::CalculateSpawnTransform() const
{
	if (SpawnPoints.Num() > 0 && !bUseCircleSpawn)
	{
		TArray<AActor*> ValidPoints;
		for (AActor* Pt : SpawnPoints)
		{
			if (IsValid(Pt))
			{
				ValidPoints.Add(Pt);
			}
		}

		if (ValidPoints.Num() > 0)
		{
			const int32 Index = FMath::RandRange(0, ValidPoints.Num() - 1);
			FTransform SpawnTrans = ValidPoints[Index]->GetActorTransform();
			FVector Loc = SpawnTrans.GetLocation();
			Loc.Z += 15.0f; // Évite que le bas de la capsule ne s'enfonce dans le sol
			SpawnTrans.SetLocation(Loc);
			return SpawnTrans;
		}
	}

	FVector CenterLoc = FVector::ZeroVector;
	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (PlayerChar)
	{
		CenterLoc = PlayerChar->GetActorLocation();
	}
	else
	{
		CenterLoc = GetActorLocation();
	}

	const float RandomAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	FVector SpawnLoc = CenterLoc + FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f) * CircleSpawnRadius;

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (PlayerChar)
	{
		QueryParams.AddIgnoredActor(PlayerChar);
	}

	const FVector TraceStart = SpawnLoc + FVector(0.0f, 0.0f, 500.0f);
	const FVector TraceEnd = SpawnLoc - FVector(0.0f, 0.0f, 1500.0f);

	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
	{
		SpawnLoc.Z = GroundHit.ImpactPoint.Z + 96.0f;
	}

	FRotator SpawnRot = FRotator::ZeroRotator;
	if (PlayerChar)
	{
		SpawnRot = (PlayerChar->GetActorLocation() - SpawnLoc).Rotation();
		SpawnRot.Pitch = 0.0f;
		SpawnRot.Roll = 0.0f;
	}

	return FTransform(SpawnRot, SpawnLoc);
}

void AEnemyWaveSpawner::OnEnemyDied(AEnemyBase* DeadEnemy)
{
	if (DeadEnemy)
	{
		ActiveEnemies.Remove(DeadEnemy);
	}
}

void AEnemyWaveSpawner::OnEnemyDeactivated(AEnemyBase* DeactivatedEnemy)
{
	if (DeactivatedEnemy)
	{
		ActiveEnemies.Remove(DeactivatedEnemy);
		InactivePool.AddUnique(DeactivatedEnemy);
	}
}

int32 AEnemyWaveSpawner::GetCurrentTargetAliveCap() const
{
	if (TimeToMaxDifficulty <= 0.0f)
	{
		return MaxAliveEnemies;
	}

	const float Progress = FMath::Clamp(ElapsedGameTime / TimeToMaxDifficulty, 0.0f, 1.0f);
	return FMath::RoundToInt(FMath::Lerp((float)MinAliveEnemies, (float)MaxAliveEnemies, Progress));
}

float AEnemyWaveSpawner::GetCurrentSpawnInterval() const
{
	if (TimeToMaxDifficulty <= 0.0f)
	{
		return MinSpawnInterval;
	}

	const float Progress = FMath::Clamp(ElapsedGameTime / TimeToMaxDifficulty, 0.0f, 1.0f);
	return FMath::Lerp(InitialSpawnInterval, MinSpawnInterval, Progress);
}
