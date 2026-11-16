// Fill out your copyright notice in the Description page of Project Settings.

#include "MusicLayer.h"
#include "MusicDataAsset.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UMusicLayer::Deinitialize()
{
	StopTrack();
	Super::Deinitialize();
}

void UMusicLayer::StartTrack(UMusicDataAsset* TrackData)
{
	if (!TrackData || !GetWorld())
	{
		return;
	}

	StopTrack();
	PisteActu = TrackData;
	BeatDurationSeconds = 60.0f / FMath::Max(TrackData->BPM, 1.0f);

	UQuartzSubsystem* Quartz = UQuartzSubsystem::Get(GetWorld());
	if (!Quartz)
	{
		return;
	}

	//crée l'horloge Quartz avec la signature rythmique donnée "BeatsPerBar"
	FQuartzClockSettings ClockSettings;
	ClockSettings.TimeSignature.NumBeats = TrackData->BeatsPerBar;
	ClockSettings.TimeSignature.BeatType = EQuartzTimeSignatureQuantization::QuarterNote;

	ClockHandle = Quartz->CreateNewClock(this, FName("MainMusicClock"), ClockSettings, true);
	if (!ClockHandle)
	{
		return;
	}

	//pointeur brut requis par les signatures UQuartzClockHandle*&
	UQuartzClockHandle* RawClockHandle = ClockHandle.Get();

	//définit la synchro au démarrage sur le prochain beat
	FQuartzQuantizationBoundary ImmediateBoundary;
	ImmediateBoundary.Quantization = EQuartzCommandQuantization::Beat;
	ImmediateBoundary.Multiplier = 1.0f;
	ImmediateBoundary.CountingReferencePoint = EQuarztQuantizationReference::BarRelative;

	FOnQuartzCommandEventBP DummyCommandDelegate;
	ClockHandle->SetBeatsPerMinute(this, ImmediateBoundary, DummyCommandDelegate, RawClockHandle, TrackData->BPM);

	//abonne l'événement à chaque beat
	FOnQuartzMetronomeEventBP MetronomeDelegate;
	MetronomeDelegate.BindUFunction(this, FName("HandleMetronomeEvent"));
	ClockHandle->SubscribeToQuantizationEvent(this, EQuartzCommandQuantization::Beat, MetronomeDelegate, RawClockHandle);

	//lance toutes les pistes en même temps sur le même tick
	BaseAudioComp = CreateAndQueuePiste(TrackData->BasePiste, 1.0f, ImmediateBoundary);

	//pistes combo (toutes muettes au départ)
	for (TObjectPtr<USoundBase> ComboSound : TrackData->ComboPistes)
	{
		if (UAudioComponent* ComboComp = CreateAndQueuePiste(ComboSound, 0.0f, ImmediateBoundary))
		{
			ComboAudioComps.Add(ComboComp);
		}
	}

	//pistes armes
	for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : TrackData->ArmePiste)
	{
		if (UAudioComponent* ArmeComp = CreateAndQueuePiste(Pair.Value, 0.0f, ImmediateBoundary))
		{
			ArmeAudioComps.Add(Pair.Key, ArmeComp);
		}
	}

	ClockHandle->StartClock(this, RawClockHandle);
	LastBeatWorldTime = GetWorld()->GetTimeSeconds();
}

void UMusicLayer::StopTrack()
{
	//nettoie le timer de fausse note
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetuneTimerHandle);
	}

	//coupe la piste principale
	if (BaseAudioComp)
	{
		BaseAudioComp->Stop();
		BaseAudioComp = nullptr;
	}

	//coupe toutes les pistes combo
	for (TObjectPtr<UAudioComponent>& ComboComp : ComboAudioComps)
	{
		if (ComboComp)
		{
			ComboComp->Stop();
		}
	}
	ComboAudioComps.Empty();

	//coupe toutes les pistes d'armes
	for (TPair<FName, TObjectPtr<UAudioComponent>>& Pair : ArmeAudioComps)
	{
		if (Pair.Value)
		{
			Pair.Value->Stop();
		}
	}
	ArmeAudioComps.Empty();

	//arrête l'horloge Quartz
	if (ClockHandle)
	{
		UQuartzClockHandle* RawClockHandle = ClockHandle.Get();
		ClockHandle->StopClock(this, true, RawClockHandle);
		ClockHandle = nullptr;
	}

	PisteActu = nullptr;
}

UAudioComponent* UMusicLayer::CreateAndQueuePiste(USoundBase* Sound, float InitialVolume, const FQuartzQuantizationBoundary& Boundary)
{
	if (!Sound || !ClockHandle)
	{
		return nullptr;
	}

	//ajoute la piste audio et la cale sur l'horloge Quartz
	UAudioComponent* AudioComp = UGameplayStatics::CreateSound2D(this, Sound, InitialVolume, 1.0f, 0.0f, nullptr, false, false);
	if (AudioComp)
	{
		UQuartzClockHandle* RawClockHandle = ClockHandle.Get();
		FQuartzQuantizationBoundary NonConstBoundary = Boundary;
		FOnQuartzCommandEventBP EmptyDelegate;
		AudioComp->PlayQuantized(this, RawClockHandle, NonConstBoundary, EmptyDelegate, 0.0f, 0.0f, InitialVolume);
	}
	return AudioComp;
}

void UMusicLayer::SetActivePisteArme(FName WeaponID, float FadeDuration)
{
	//monte le volume de l'arme active et coupe les autres en fondu
	for (TPair<FName, TObjectPtr<UAudioComponent>>& Pair : ArmeAudioComps)
	{
		if (Pair.Value)
		{
			const float TargetVolume = (Pair.Key == WeaponID) ? 1.0f : 0.0f;
			Pair.Value->AdjustVolume(FadeDuration, TargetVolume);
		}
	}
}

void UMusicLayer::SetComboPalier(int32 Palier, float FadeDuration)
{
	//active les pistes d'index < Palier (ex: Palier 2 active index 0 et 1)
	for (int32 i = 0; i < ComboAudioComps.Num(); ++i)
	{
		if (ComboAudioComps[i])
		{
			const float TargetVolume = (i < Palier) ? 1.0f : 0.0f;
			ComboAudioComps[i]->AdjustVolume(FadeDuration, TargetVolume);
		}
	}
}

void UMusicLayer::SetDetunePitch(float PitchMultiplier)
{
	const float ClampedPitch = FMath::Clamp(PitchMultiplier, 0.25f, 2.0f);

	//applique le pitch sur les pistes d'armes
	for (TPair<FName, TObjectPtr<UAudioComponent>>& Pair : ArmeAudioComps)
	{
		if (Pair.Value)
		{
			Pair.Value->SetPitchMultiplier(ClampedPitch);
		}
	}
}

void UMusicLayer::HandleRateNote()
{
	//coupe toutes les pistes de combo
	SetComboPalier(0, 0.1f);

	//désaccorde la piste de l'arme pendant 0.3s puis remet le pitch normal
	SetDetunePitch(0.85f);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetuneTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(DetuneTimerHandle, [this]()
		{
			SetDetunePitch(1.0f);
		}, 0.3f, false);
	}
}


bool UMusicLayer::IsOnBeat(float ToleranceSec) const
{
	if (!GetWorld() || !ClockHandle)
	{
		return false;
	}

	//réutilise directement GetBeatOffsetMs() pour garantir exactement la même logique
	return FMath::Abs(GetBeatOffsetMs()) <= (ToleranceSec * 1000.0f);
}

void UMusicLayer::HandleMetronomeEvent(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	if (GetWorld())
	{
		LastBeatWorldTime = GetWorld()->GetTimeSeconds();
	}
	OnBeat.Broadcast(NumBars, Beat);
}

float UMusicLayer::GetBeatOffsetMs() const
{
	if (!GetWorld() || !ClockHandle)
	{
		return 0.0f;
	}

	//soustrait la latence audio au temps actuel pour aligner l'oreille du joueur avec l'horloge
	const double AdjustedTime = GetWorld()->GetTimeSeconds() - static_cast<double>(AudioLatencyOffsetSec);
	const double TimeSinceLastBeat = AdjustedTime - LastBeatWorldTime;
	const double TimeToNextBeat = BeatDurationSeconds - TimeSinceLastBeat;

	if (TimeSinceLastBeat <= TimeToNextBeat)
	{
		return static_cast<float>(TimeSinceLastBeat * 1000.0);
	}
	return static_cast<float>(-TimeToNextBeat * 1000.0);
}
