// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "FinalProjectCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerShootRhythm, float, OffsetMs, float, ToleranceMaxMs);

UCLASS(abstract)
class AFinalProjectCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

protected:
	
	void BeginPlay();

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;
	
	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SprintAction;

	/** Slide Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* SlideAction;
	
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* TirAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> SwitchArme1Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> SwitchArme2Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<class UInputAction> SwitchArme3Action;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Timing", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float BeatToleranceSec = 0.12f;
	
	//fonction appelée à chaque beat par MusicLayer
	UFUNCTION()
	void OnMusicBeatReceived(int32 Bar, int32 Beat);

	//effet FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rhythm", meta = (ClampMin = "0.0", ClampMax = "15.0"))
	float BeatFovKickAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rhythm", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float FovInterpSpeed = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Rhythm")
	bool bAccentuateFirstBeat = true;

	float BaseFOV = 90.0f;
	float BaseFirstPersonFOV = 70.0f;
	float CurrentFovOffset = 0.0f;
	
public:
	AFinalProjectCharacter();

	// --- Santé du Joueur ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 100.0f;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount) { CurrentHealth = FMath::Clamp(CurrentHealth + HealAmount, 0.0f, MaxHealth); }

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm")
	FOnPlayerShootRhythm OnPlayerShoot;
	
	// --- Vitesses et Sprint ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint")
	float WalkSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint")
	float SprintSpeed = 650.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Sprint")
	bool bIsSprinting = false;

	// --- Glissade (Slide) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideImpulse = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideDuration = 0.8f;

	float DefaultGroundFriction = 8.0f;
	float DefaultBrakingDeceleration = 2048.0f;
	float DefaultMaxWalkSpeedCrouched = 200.0f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Slide")
	bool bIsSliding = false;
	
	FTimerHandle SlideTimerHandle;

	float TargetSlideRoll = 0.0f;
	float CurrentSlideRoll = 0.0f;
	
	FVector BaseCameraLocation = FVector(-2.8f, 5.89f, 0.0f);
	float CurrentSlideDown = 0.0f;
	float TargetSlideDown = 0.0f;
	
	float NormalCameraZ = 65.0f;
	float SlideCameraZ = 15.0f;
	float CurrentCameraZ = 65.0f;

	// --- Répulsion des ennemis et esquive pendant la glissade ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideRepulseRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideRepulseForce = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideRepulseUpwardForce = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideCapsuleRadius = 20.0f;

	float DefaultCapsuleRadius = 34.0f;

	TSet<TWeakObjectPtr<AActor>> SlidePushedActors;

	void PerformSlideRepulse();
	
	void StartSprint();
	void StopSprint();
	void StartSlide();
	void StopSlide();
	
	virtual void Tick(float DeltaSeconds) override;

protected:

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFire();
		
public:
	// --- Stats & Kills ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 KillCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void AddKill() { ++KillCount; }

	//instruments et changement d'arme
	void ChangerArme(FName WeaponID);
	void SelectGuitar()  { ChangerArme(TEXT("guitar")); }
	void SelectTrumpet() { ChangerArme(TEXT("trumpet")); }
	void SelectPiano()   { ChangerArme(TEXT("piano")); }

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

