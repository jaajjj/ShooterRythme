#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmCrosshairWidget.generated.h"

class UCanvasPanel;
class URhythmTickWidget;

/**
 * Widget de base pour le réticule rythmique (Cercle central + Point + Barres).
 * Gère le pool dynamique de barres et leur déplacement GPU pur selon la musique.
 */
UCLASS(Abstract)
class FINALPROJECT_API URhythmCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmCrosshairWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//calcule la distance par rapport au centre et l'opacité d'une barre selon la musique
	UFUNCTION(BlueprintPure, Category = "Rhythm Math")
	void CalculateTickData(int32 NoteIndex, float InDistanceMax, int32 InNotesVisibles, float& OutDistance, float& OutOpacity, bool& bOutVisible) const;

	//retourne la couleur associée à un instrument (trumpet, guitar, piano...)
	UFUNCTION(BlueprintPure, Category = "Rhythm Math")
	FLinearColor GetCouleurInstrument(FName ArmeID) const;

	//applique la couleur de l'instrument actif à toutes les barres du pool
	UFUNCTION(BlueprintCallable, Category = "Rhythm UI")
	void ChangerCouleurInstrument(FName ArmeID);

protected:
	//le canvas contenant les barres (cocher Is Variable sur le Canvas Panel dans le WBP)
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Rhythm UI")
	TObjectPtr<UCanvasPanel> NotesCanvas;

	//la classe de widget à instancier dans le pool (WBP_RhythmTick)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Config")
	TSubclassOf<URhythmTickWidget> TickWidgetClass;

	//nombre de notes visibles d'avance (crée dynamiquement le pool correspondant)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Config", meta = (ClampMin = "1", ClampMax = "5"))
	int32 NotesVisibles = 3;

	//distance max par défaut depuis le centre (en pixels)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Config")
	float DistanceMax = 350.0f;

	//couleurs associées à chaque instrument
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Config")
	TMap<FName, FLinearColor> CouleursArmes;

	//couleur par défaut si l'instrument n'est pas dans la liste
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Config")
	FLinearColor CouleurParDefaut = FLinearColor(1.0f, 0.78f, 0.1f, 1.0f);

private:
	//pool dynamique de barres (créé une seule fois à l'initialisation selon NotesVisibles)
	UPROPERTY()
	TArray<TObjectPtr<URhythmTickWidget>> PoolTicksGauche;

	UPROPERTY()
	TArray<TObjectPtr<URhythmTickWidget>> PoolTicksDroite;

	FLinearColor CouleurActuelle;
};
