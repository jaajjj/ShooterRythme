// Copyright Epic Games, Inc. All Rights Reserved.

#include "FinalProjectCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FinalProject.h"
#include "Music/MusicLayer.h"
#include "FinalProjectGameMode.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AFinalProjectCharacter::AFinalProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->SetCrouchedHalfHeight(44.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	DefaultGroundFriction = GetCharacterMovement()->GroundFriction;
	DefaultBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	DefaultMaxWalkSpeedCrouched = GetCharacterMovement()->MaxWalkSpeedCrouched;
	
	PrimaryActorTick.bCanEverTick = true;
}

void AFinalProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (FirstPersonCameraComponent)
	{
		BaseFOV = FirstPersonCameraComponent->FieldOfView;
		BaseFirstPersonFOV = FirstPersonCameraComponent->FirstPersonFieldOfView;
	}

	if (UMusicLayer* Music = GetWorld()->GetSubsystem<UMusicLayer>())
	{
		Music->OnBeat.AddDynamic(this, &AFinalProjectCharacter::OnMusicBeatReceived);
	}
}

void AFinalProjectCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FirstPersonCameraComponent && FMath::Abs(CurrentFovOffset) > 0.005f)
	{
		CurrentFovOffset = FMath::FInterpTo(CurrentFovOffset, 0.0f, DeltaSeconds, FovInterpSpeed);
		FirstPersonCameraComponent->SetFieldOfView(BaseFOV + CurrentFovOffset);
		FirstPersonCameraComponent->SetFirstPersonFieldOfView(BaseFirstPersonFOV + CurrentFovOffset);
	}
	// if (FirstPersonCameraComponent)
	// {
	// 	const float TargetCameraZ = bIsSliding ? SlideCameraZ : NormalCameraZ;
	// 	CurrentCameraZ = FMath::FInterpTo(CurrentCameraZ, TargetCameraZ, DeltaSeconds, 12.0f);
	// 	
	// 	FVector CamLoc = FirstPersonCameraComponent->GetRelativeLocation();
	// 	CamLoc.Z = CurrentCameraZ;
	// 	FirstPersonCameraComponent->SetRelativeLocation(CamLoc);
	// }
	CurrentSlideRoll = FMath::FInterpTo(CurrentSlideRoll, TargetSlideRoll, DeltaSeconds, 8.0f);
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FRotator ControlRot = PC->GetControlRotation();
		ControlRot.Roll = CurrentSlideRoll;
		PC->SetControlRotation(ControlRot);
	}
}


void AFinalProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AFinalProjectCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFinalProjectCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::LookInput);

		//tire
		if (TirAction)
		{
			EnhancedInputComponent->BindAction(TirAction, ETriggerEvent::Started, this, &AFinalProjectCharacter::DoFire);
		}
		
		// changement d'arme
		if (SwitchArme1Action)
		{
			EnhancedInputComponent->BindAction(SwitchArme1Action, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::SelectGuitar);
		}

		if (SwitchArme2Action)
		{
			EnhancedInputComponent->BindAction(SwitchArme2Action, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::SelectTrumpet);
		}

		if (SwitchArme3Action)
		{
			EnhancedInputComponent->BindAction(SwitchArme3Action, ETriggerEvent::Triggered, this, &AFinalProjectCharacter::SelectPiano);
		}
		
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AFinalProjectCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFinalProjectCharacter::StopSprint);
		}

		if (SlideAction)
		{
			EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AFinalProjectCharacter::StartSlide);
		}
	}
	else
	{
		UE_LOG(LogFinalProject, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AFinalProjectCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AFinalProjectCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AFinalProjectCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		// pass the rotation inputs
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AFinalProjectCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AFinalProjectCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AFinalProjectCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AFinalProjectCharacter::DoFire()
{
	UMusicLayer* Music = GetWorld()->GetSubsystem<UMusicLayer>();
	AFinalProjectGameMode* GM = Cast<AFinalProjectGameMode>(GetWorld()->GetAuthGameMode());
	
	if (!Music || !GM)
	{
		return;
	}

	const float OffsetMs = Music->GetBeatOffsetMs();
	OnPlayerShoot.Broadcast(OffsetMs, BeatToleranceSec * 1000.0f);

	//vérifie si le tir tombe dans la fenêtre du beat (0.08s = +-80ms)
	if (Music->IsOnBeat(BeatToleranceSec))
	{
		GM->AjouterCombo();
		//debug
		if (GEngine)
		{
			const FString Msg = FString::Printf(TEXT("ON BEAT (%+.0f ms) | Combo : %d"), OffsetMs, GM->GetCombo());
			GEngine->AddOnScreenDebugMessage(1, 1.5f, FColor::Green, Msg);
		}
	}
	else
	{
		GM->ResetCombo();
		//debug
		if (GEngine)
		{
			const FString Msg = FString::Printf(TEXT("RATE (%+.0f ms) | Combo : 0"), OffsetMs);
			GEngine->AddOnScreenDebugMessage(1, 1.5f, FColor::Red, Msg);
		}
	}
}

void AFinalProjectCharacter::OnMusicBeatReceived(int32 Bar, int32 Beat)
{
	// Effet FOV Kick rythmique à chaque beat
	if (BeatFovKickAmount > 0.0f && FirstPersonCameraComponent)
	{
		const float Multiplier = (bAccentuateFirstBeat && Beat == 1) ? 1.4f : 1.0f;
		CurrentFovOffset = -BeatFovKickAmount * Multiplier;
		FirstPersonCameraComponent->SetFieldOfView(BaseFOV + CurrentFovOffset);
		FirstPersonCameraComponent->SetFirstPersonFieldOfView(BaseFirstPersonFOV + CurrentFovOffset);
	}
}


void AFinalProjectCharacter::ChangerArme(FName WeaponID)
{
	if (UMusicLayer* Music = GetWorld()->GetSubsystem<UMusicLayer>())
	{
		Music->SetActivePisteArme(WeaponID, 0.15f);
		//debug
		if (GEngine)
		{
			const FString Msg = FString::Printf(TEXT("ARME ACTIVE : %s"), *WeaponID.ToString());
			GEngine->AddOnScreenDebugMessage(3, 2.0f, FColor::Yellow, Msg);
		}
	}
}

void AFinalProjectCharacter::StartSprint()
{
	bIsSprinting = true;
	if (!bIsSliding)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
		//debug
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(20, 1.0f, FColor::Cyan, FString::Printf(TEXT("SPRINT ACTIF - Vitesse: %f"), GetCharacterMovement()->MaxWalkSpeed));
		}
	}
}

void AFinalProjectCharacter::StopSprint()
{
	bIsSprinting = false;
	if (!bIsSliding)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		//debug
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(20, 1.0f, FColor::Orange, FString::Printf(TEXT("MARCHE NORMALE - Vitesse: %f"), GetCharacterMovement()->MaxWalkSpeed));
		}
	}
}

void AFinalProjectCharacter::StartSlide()
{
	if (!GetCharacterMovement()->IsMovingOnGround() || bIsSliding)
	{
		return;
	}

	if (GetVelocity().Size() < (WalkSpeed * 0.7f))
	{
		return;
	}

	bIsSliding = true;
	TargetSlideRoll = -6.0f;

	Crouch();

	GetCharacterMovement()->GroundFriction = 0.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 0.0f;

	GetCharacterMovement()->MaxWalkSpeedCrouched = SlideImpulse;

	FVector SlideDirection = GetVelocity().GetSafeNormal2D();
	if (SlideDirection.IsNearlyZero())
	{
		SlideDirection = GetActorForwardVector();
	}
	LaunchCharacter(SlideDirection * SlideImpulse, true, false);

	GetWorld()->GetTimerManager().SetTimer(
		SlideTimerHandle,
		this,
		&AFinalProjectCharacter::StopSlide,
		SlideDuration,
		false
	);
}

void AFinalProjectCharacter::StopSlide()
{
	if (!bIsSliding)
	{
		return;
	}

	bIsSliding = false;
	TargetSlideRoll = 0.0f;
	GetWorld()->GetTimerManager().ClearTimer(SlideTimerHandle);

	UnCrouch();
	GetCharacterMovement()->GroundFriction = DefaultGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = DefaultBrakingDeceleration;
	GetCharacterMovement()->MaxWalkSpeedCrouched = DefaultMaxWalkSpeedCrouched;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

float AFinalProjectCharacter::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f || CurrentHealth <= 0.0f)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.0f, MaxHealth);
	//debug
	UE_LOG(LogTemplateCharacter, Log, TEXT("Player took %f damage, CurrentHealth: %f"), ActualDamage, CurrentHealth);
	//debug
	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Player is DEAD!"));
	}

	return ActualDamage;
}
