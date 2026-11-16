// Copyright Epic Games, Inc. All Rights Reserved.

#include "FinalProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "FinalProjectCameraManager.h"

AFinalProjectPlayerController::AFinalProjectPlayerController()
{
	//assigne la caméra FPS personnalisée
	PlayerCameraManagerClass = AFinalProjectCameraManager::StaticClass();
}

void AFinalProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	//ajoute tous les mapping contexts configurés
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			if (CurrentContext)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}
		}
	}
}