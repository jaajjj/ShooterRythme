// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Character/EnemyDataAsset.h"
#include "EnemyBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeathSignature, AEnemyBase*, DeadEnemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyDeactivatedSignature, AEnemyBase*, DeactivatedEnemy);

class AAIController;
class ADetourCrowdAIController;
class UAnimMontage;

UCLASS()
class FINALPROJECT_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	AEnemyBase();

	// Data Asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Data")
	TObjectPtr<UEnemyDataAsset> EnemyData;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Data")
	virtual void ApplyEnemyData(UEnemyDataAsset* InData);

	// --- Stats en jeu ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Stats")
	float CurrentHealth = 100.0f;

	// --- Combat ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float AttackHitDelay = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Combat")
	float DeathLifeSpan = 4.0f;

	// --- Animations ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// Liste des montages d'attaque (sélection aléatoire à chaque coup)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	// --- État ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|State")
	bool bIsAttacking = false;

	// Object Pooling
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Pool")
	bool bIsPooled = false;

	// Event Broadcast
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDeathSignature OnDeathDelegate;

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Events")
	FOnEnemyDeactivatedSignature OnDeactivatedDelegate;

	// Fonctions Object Pooling
	UFUNCTION(BlueprintCallable, Category = "Enemy|Pool")
	virtual void ActivateFromPool(UEnemyDataAsset* InData, const FTransform& SpawnTransform);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Pool")
	virtual void DeactivateToPool();

	// Event Blueprint
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnEnemyAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnEnemyTakeDamage(float DamageTaken, AActor* DamageCauser);

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Events")
	void OnEnemyDeath();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|AI")
	float ChaseInterval = 0.25f;

	FTimerHandle ChaseTimerHandle;
	FTimerHandle AttackCooldownTimerHandle;
	FTimerHandle HitDelayTimerHandle;
	FTimerHandle ReturnToPoolTimerHandle;

	UPROPERTY()
	TObjectPtr<AActor> CurrentAttackTarget;

	UPROPERTY()
	TObjectPtr<AAIController> CachedAIController;

public:
	// Gestion dégâts
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Déplacement et traque
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void UpdateChase();

	// Attaque
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void PerformAttack(AActor* Target);

	// Retourne un montage d'attaque au hasard parmi la liste (ou AttackMontage par défaut)
	UFUNCTION(BlueprintPure, Category = "Enemy|Combat")
	UAnimMontage* GetRandomAttackMontage() const;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void DealDamage();

	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void ResetAttack();

	// Mort
	UFUNCTION(BlueprintCallable, Category = "Enemy|Combat")
	virtual void Die();
};
