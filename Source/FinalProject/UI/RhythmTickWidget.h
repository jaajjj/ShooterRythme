#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmTickWidget.generated.h"

class UImage;

/**
 * Widget représentant une barre rythmique individuelle.
 * Le visuel (Image, brush, couleur) est configuré dans le Widget Blueprint (WBP_RhythmTick).
 */
UCLASS(Abstract)
class FINALPROJECT_API URhythmTickWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//change la couleur de la barre selon l'instrument
	UFUNCTION(BlueprintCallable, Category = "Rhythm UI")
	void SetTickColor(FLinearColor NouvelleCouleur);

	//gère le fondu d'opacité de la barre
	UFUNCTION(BlueprintCallable, Category = "Rhythm UI")
	void SetTickOpacity(float Opacite);

protected:
	//image de la barre dans le Widget BP (nommée TickImage)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Rhythm UI")
	TObjectPtr<UImage> TickImage;
};
