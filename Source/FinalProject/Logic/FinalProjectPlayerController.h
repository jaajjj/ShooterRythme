// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FinalProjectPlayerController.generated.h"

class UInputMappingContext;

UCLASS(abstract, config="Game")
class FINALPROJECT_API AFinalProjectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFinalProjectPlayerController();

protected:
	//contexts d'inputs actifs (IMC_Default, IMC_MouseLook, etc.)
	UPROPERTY(EditAnywhere, Category="Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	virtual void SetupInputComponent() override;
};