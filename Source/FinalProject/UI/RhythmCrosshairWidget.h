#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RhythmCrosshairWidget.generated.h"

class UCanvasPanel;
class URhythmTickWidget;


UCLASS(Abstract)
class FINALPROJECT_API URhythmCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URhythmCrosshairWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	//joue l'animation de pulsation passive du tempo
	UFUNCTION(BlueprintCallable, Category = "Rhythm Animations")
	void PlayBeatPulse();

	//joue l'animation de tir réussi (effet de morsure + onde de choc)
	UFUNCTION(BlueprintCallable, Category = "Rhythm Animations")
	void PlayHitSuccess();

	//joue l'animation de tir raté (secousse rouge punitive)
	UFUNCTION(BlueprintCallable, Category = "Rhythm Animations")
	void PlayHitMiss();

	//calcule la distance par rapport au centre et l'opacité d'une barre selon la musique
	UFUNCTION(BlueprintPure, Category = "Rhythm Math")
	void CalculateTickData(int32 NoteIndex, float InDistanceMax, int32 InNotesVisibles, float& OutDistance, float& OutOpacity, bool& bOutVisible) const;

	//calcule l'état dynamique complet d'une note (distance, opacité, échelle et couleur)
	UFUNCTION(BlueprintPure, Category = "Rhythm Math")
	void CalculateTickFull(int32 NoteIndex, float InDistanceMax, int32 InNotesVisibles, float& OutDistance, float& OutOpacity, float& OutScale, FLinearColor& OutColor, bool& bOutVisible) const;

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

	//distance d'impact depuis le centre où les notes touchent le récepteur (en pixels, correspond à la position X des récepteurs)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Config")
	float ImpactDistance = 60.0f;

	//couleurs associées à chaque instrument
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Config")
	TMap<FName, FLinearColor> CouleursArmes;

	//couleur par défaut si l'instrument n'est pas dans la liste
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Config")
	FLinearColor CouleurParDefaut = FLinearColor(1.0f, 0.78f, 0.1f, 1.0f);

	//animations UMG (automatiquement reliées si créées avec ces noms dans le WBP)
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "Rhythm Animations")
	TObjectPtr<UWidgetAnimation> Anim_BeatPulse;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "Rhythm Animations")
	TObjectPtr<UWidgetAnimation> Anim_HitSuccess;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional), BlueprintReadOnly, Category = "Rhythm Animations")
	TObjectPtr<UWidgetAnimation> Anim_HitMiss;

	UFUNCTION()
	void HandleMusicBeat(int32 Bar, int32 Beat);

	UFUNCTION()
	void HandlePlayerShoot(float OffsetMs, float ToleranceMaxMs);

private:
	//pool dynamique de barres (créé une seule fois à l'initialisation selon NotesVisibles)
	UPROPERTY()
	TArray<TObjectPtr<URhythmTickWidget>> PoolTicksGauche;

	UPROPERTY()
	TArray<TObjectPtr<URhythmTickWidget>> PoolTicksDroite;

	FLinearColor CouleurActuelle;
	bool bCharacterBound = false;

	void EnsurePoolSize(int32 DesiredSize);
	void BindCharacterEvents();
};
