// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "EnemyDataAsset.generated.h"

class USkeletalMesh;
class UAnimInstance;
class UAnimMontage;


UCLASS(BlueprintType)
class FINALPROJECT_API UEnemyDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Nom 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText EnemyName;

	// --- Visuel & Mesh ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshRelativeLocation = FVector(0.0f, 0.0f, -96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FRotator MeshRelativeRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector MeshRelativeScale = FVector(1.0f, 1.0f, 1.0f);

	// --- Animations  ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Liste des montages d'attaque possibles
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	// --- Stats & Combat ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "50.0"))
	float MovementSpeed = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0"))
	float AttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "50.0"))
	float AttackRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.2"))
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackHitDelay = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.5"))
	float DeathLifeSpan = 4.0f;
};


USTRUCT(BlueprintType)
struct FINALPROJECT_API FEnemyWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	// Numéro vague
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 WaveNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TObjectPtr<UEnemyDataAsset> EnemyDataAsset;

	// Nombre d'ennemis de ce type à faire apparaître
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 SpawnCount = 5;

	// Délai en secondes entre chaque apparition dans cette vague
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float SpawnInterval = 1.5f;

	// Délai d'attente avant le début de cette vague
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	float InitialDelay = 2.0f;
};
