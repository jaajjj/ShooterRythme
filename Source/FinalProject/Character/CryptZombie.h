// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CryptZombie.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCryptZombieDeathSignature);

class AAIController;
class UAnimMontage;


UCLASS()
class FINALPROJECT_API ACryptZombie : public ACharacter
{
	GENERATED_BODY()

public:
	ACryptZombie();

	// --- Santé & Stats ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Stats")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|Stats")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Stats")
	int32 ScoreValue = 10;

	// --- Combat & Attaque ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
	float AttackDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
	float AttackRange = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
	float AttackCooldown = 1.5f;

	// Délai avant impact du coup ( pour matcher avec l'animation tia capté)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
	float AttackHitDelay = 1.0f;

	// --- Animations  ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Animation")
	TObjectPtr<UAnimMontage> HitMontage;

	// --- Durée avant disparition du cadavre ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Combat")
	float DeathLifeSpan = 4.0f;

	// --- État ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|State")
	bool bIsAttacking = false;

	// Event Broadcast
	UPROPERTY(BlueprintAssignable, Category = "Zombie|Events")
	FOnCryptZombieDeathSignature OnDeathDelegate;

	// Autre Event
	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void OnZombieAttack(AActor* Target);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void OnZombieTakeDamage(float DamageTaken, AActor* DamageCauser);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zombie|Events")
	void OnZombieDeath();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Timer chase
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|AI")
	float ChaseInterval = 0.25f;

	FTimerHandle ChaseTimerHandle;
	FTimerHandle AttackCooldownTimerHandle;
	FTimerHandle HitDelayTimerHandle;

	UPROPERTY()
	TObjectPtr<AActor> CurrentAttackTarget;

	UPROPERTY()
	TObjectPtr<AAIController> CachedAIController;

public:
	// Gestion degats reçus 
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	// Logique traque
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void UpdateChase();

	// Logique attaque
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void PerformAttack(AActor* Target);

	// Applique reellement les degats au moment de l'impact
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void DealDamage();

	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void ResetAttack();

	// Logique mort
	UFUNCTION(BlueprintCallable, Category = "Zombie|Combat")
	void Die();
};
