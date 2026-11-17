#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Sound/QuartzQuantizationUtilities.h"
#include "MusicLayer.generated.h"

class UMusicDataAsset;
class UQuartzClockHandle;
class UAudioComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMusicBeatSignature, int32, Bar, int32, Beat);

UCLASS()
class FINALPROJECT_API UMusicLayer : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//Déclenché à chaque beat
	UPROPERTY(BlueprintAssignable, Category = "Music")
	FOnMusicBeatSignature OnBeat;

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StartTrack(UMusicDataAsset* TrackData);

	UFUNCTION(BlueprintCallable, Category = "Music")
	void StopTrack();

	//active la piste de l'arme instru
	UFUNCTION(BlueprintCallable, Category = "Music")
	void SetActivePisteArme(FName WeaponID, float FadeDuration = 0.15f);

	//autorise ou coupe l'écoute de l'instrument d'arme (ex: déclenché à partir de 5 combos)
	UFUNCTION(BlueprintCallable, Category = "Music")
	void SetArmeAudible(bool bAudible, float FadeDuration = 0.2f);

	//active les pistes combo selon le palier atteint (0 = aucune, 1 = piste 1, 2 = pistes 1+2, 3 = pistes 1+2+3)
	UFUNCTION(BlueprintCallable, Category = "Music")
	void SetComboPalier(int32 Palier, float FadeDuration = 0.25f);

	//handler appelé quand le joueur rate une note (coupe les combos et désaccorde l'arme temporairement)
	UFUNCTION(BlueprintCallable, Category = "Music")
	void HandleRateNote();

	//true si player tape au bon moment (avec tolerence)
	UFUNCTION(BlueprintPure, Category = "Music Timing")
	bool IsOnBeat(float ToleranceSec = 0.12f) const;
	
	UFUNCTION(BlueprintPure, Category = "Music Timing")
	float GetBeatOffsetMs() const;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music Timing")
	float AudioLatencyOffsetSec = 0.055f;

protected:
	virtual void Deinitialize() override;

private:
	UAudioComponent* CreateAndQueuePiste(USoundBase* Sound, float InitialVolume, const FQuartzQuantizationBoundary& Boundary);

	UFUNCTION()
	void HandleMetronomeEvent(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);

	UPROPERTY()
	TObjectPtr<UMusicDataAsset> PisteActu;

	UPROPERTY()
	TObjectPtr<UQuartzClockHandle> ClockHandle;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BaseAudioComp;

	UPROPERTY()
	TArray<TObjectPtr<UAudioComponent>> ComboAudioComps;

	UPROPERTY()
	TMap<FName, TObjectPtr<UAudioComponent>> ArmeAudioComps;
	

	void AppliquerVolumeArmes(float FadeDuration);
	FName ArmeActiveID = NAME_None;
	bool bArmeAudible = false;

	FTimerHandle DetuneTimerHandle;
	double LastBeatWorldTime = 0.0;
	float BeatDurationSeconds = 0.5f;
};