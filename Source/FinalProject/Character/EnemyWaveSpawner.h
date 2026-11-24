// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Character/EnemyBase.h"
#include "Character/EnemyDataAsset.h"
#include "EnemyWaveSpawner.generated.h"


USTRUCT(BlueprintType)
struct FEnemyTierConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	FString EnemyName = TEXT("Monstre");

	// Le DataAsset du monstre )
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TObjectPtr<UEnemyDataAsset> EnemyData;

	// Temps de jeu en secondes avant que ce monstre ne commence à apparaître
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float UnlockTime = 0.0f;

	// Poids d'apparition relatif (probabilité) une fois débloqué
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float SpawnWeight = 1.0f;
};

UCLASS()
class FINALPROJECT_API AEnemyWaveSpawner : public AActor
{
	GENERATED_BODY()

public:
	AEnemyWaveSpawner();

	// --- Configuration des Monstres  ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Enemies")
	TArray<FEnemyTierConfig> EnemyTiers;

	// Classe de base des ennemis à faire spawner (laisser AEnemyBase ou assigner BP_EnemyBase)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Pooling")
	TSubclassOf<AEnemyBase> EnemyBaseClass;

	// --- Plafond d'ennemis vivants  ---
	// Nombre maximum d'ennemis vivants au tout début
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Difficulty", meta = (ClampMin = "1"))
	int32 MinAliveEnemies = 10;

	// Nombre maximum d'ennemis vivants au pic de la difficulté
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Difficulty", meta = (ClampMin = "5"))
	int32 MaxAliveEnemies = 100;

	// Temps en secondes pour atteindre le pic de difficulté 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Difficulty", meta = (ClampMin = "60.0"))
	float TimeToMaxDifficulty = 600.0f;

	// --- Rythme de Spawn ---
	// Délai en secondes entre chaque apparition au début
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Spawning", meta = (ClampMin = "0.1"))
	float InitialSpawnInterval = 2.0f;

	// Délai minimum entre chaque apparition quand la horde est à son max
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Spawning", meta = (ClampMin = "0.05"))
	float MinSpawnInterval = 0.4f;

	// --- Lieux d'apparition ---
	// Balises / Portes d'arène fixes. Si vide, utilise le spawn en cercle automatiquement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|SpawnLocation")
	TArray<TObjectPtr<AActor>> SpawnPoints;

	// Forcer l'apparition en cercle autour de l'arène 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|SpawnLocation")
	bool bUseCircleSpawn = false;

	// Rayon du cercle de spawn autour du joueur (en cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|SpawnLocation", meta = (ClampMin = "500.0"))
	float CircleSpawnRadius = 1600.0f;

	// --- Object Pooling ---
	// Nombre d'ennemis pré-instanciés au lancement de la partie
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave|Pooling", meta = (ClampMin = "5"))
	int32 InitialPoolSize = 20;

	// --- État en direct  ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave|Status")
	float ElapsedGameTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave|Status")
	int32 CurrentAliveCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wave|Status")
	int32 CurrentAllowedMaxAlive = 10;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Liste des ennemis inactifs en réserve dans le pool
	UPROPERTY()
	TArray<TObjectPtr<AEnemyBase>> InactivePool;

	// Liste des ennemis actuellement vivants et actifs dans l'arène
	UPROPERTY()
	TArray<TObjectPtr<AEnemyBase>> ActiveEnemies;

	FTimerHandle SpawnTimerHandle;

	// Fonctions internes
	void InitializePool();
	AEnemyBase* GetOrCreateEnemyFromPool();
	void TriggerSpawnCycle();
	void SpawnSingleEnemy();

	UEnemyDataAsset* ChooseEnemyDataByTime() const;
	FTransform CalculateSpawnTransform() const;

	UFUNCTION()
	void OnEnemyDied(AEnemyBase* DeadEnemy);

	UFUNCTION()
	void OnEnemyDeactivated(AEnemyBase* DeactivatedEnemy);

public:
	// Calcule le plafond actuel d'ennemis vivants selon le temps écoulé
	UFUNCTION(BlueprintPure, Category = "Wave|Status")
	int32 GetCurrentTargetAliveCap() const;

	// Calcule le délai de spawn actuel selon le temps écoulé
	UFUNCTION(BlueprintPure, Category = "Wave|Status")
	float GetCurrentSpawnInterval() const;
};
