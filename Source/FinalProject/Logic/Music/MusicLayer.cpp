// Fill out your copyright notice in the Description page of Project Settings.

#include "MusicLayer.h"
#include "MusicDataAsset.h"
#include "Quartz/QuartzSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "../Settings/RhythmSettings.h"

namespace
{
	constexpr float SilentVolume = 0.0001f;
}

void UMusicLayer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const URhythmSettings* Settings = GetDefault<URhythmSettings>())
	{
		AudioLatencyOffsetSec = FMath::Clamp(Settings->AudioLatencyOffsetSec, 0.0f, 0.5f);
	}
}

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

	//pistes combo (toutes muettes au départ mais actives sur l'horloge à SilentVolume)
	for (TObjectPtr<USoundBase> ComboSound : TrackData->ComboPistes)
	{
		if (UAudioComponent* ComboComp = CreateAndQueuePiste(ComboSound, SilentVolume, ImmediateBoundary))
		{
			ComboAudioComps.Add(ComboComp);
		}
	}

	//pistes armes
	for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : TrackData->ArmePiste)
	{
		if (UAudioComponent* ArmeComp = CreateAndQueuePiste(Pair.Value, SilentVolume, ImmediateBoundary))
		{
			ArmeAudioComps.Add(Pair.Key, ArmeComp);
		}
	}

	ClockHandle->StartClock(this, RawClockHandle);
	LastBeatWorldTime = GetWorld()->GetTimeSeconds();
}

void UMusicLayer::StopTrack()
{
	//nettoie le timer de fausse note et de synchro de beat
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetuneTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(BeatLatencyTimerHandle);
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
	UAudioComponent* AudioComp = UGameplayStatics::CreateSound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, false, false);
	if (AudioComp)
	{
		AudioComp->bAutoDestroy = false;
		AudioComp->SetVolumeMultiplier(1.0f);
		UQuartzClockHandle* RawClockHandle = ClockHandle.Get();
		FQuartzQuantizationBoundary NonConstBoundary = Boundary;
		FOnQuartzCommandEventBP EmptyDelegate;
		AudioComp->PlayQuantized(this, RawClockHandle, NonConstBoundary, EmptyDelegate, 0.0f, 0.0f, InitialVolume);
	}
	return AudioComp;
}

void UMusicLayer::SetActivePisteArme(FName WeaponID, float FadeDuration)
{
	ArmeActiveID = WeaponID;
	AppliquerVolumeArmes(FadeDuration);
}

void UMusicLayer::SetArmeAudible(bool bAudible, float FadeDuration)
{
	if (bArmeAudible != bAudible)
	{
		bArmeAudible = bAudible;
		AppliquerVolumeArmes(FadeDuration);
	}
}

void UMusicLayer::AppliquerVolumeArmes(float FadeDuration)
{
	//si bArmeAudible est vrai (combo >= 5), l'instrument sélectionné joue à 1.0f, sinon tout est muet
	for (TPair<FName, TObjectPtr<UAudioComponent>>& Pair : ArmeAudioComps)
	{
		if (Pair.Value)
		{
			if (!Pair.Value->IsActive() || !Pair.Value->IsPlaying())
			{
				Pair.Value->Play();
			}
			const float TargetVolume = (bArmeAudible && Pair.Key == ArmeActiveID) ? 1.0f : SilentVolume;
			Pair.Value->AdjustVolume(FadeDuration, TargetVolume);
		}
	}
}

void UMusicLayer::SetComboPalier(int32 Palier, float FadeDuration)
{
	//seul le palier correspondant joue, les autres se mutent
	for (int32 i = 0; i < ComboAudioComps.Num(); ++i)
	{
		if (ComboAudioComps[i])
		{
			if (!ComboAudioComps[i]->IsActive() || !ComboAudioComps[i]->IsPlaying())
			{
				ComboAudioComps[i]->Play();
			}
			const float TargetVolume = (Palier > 0 && i == (Palier - 1)) ? 1.0f : SilentVolume;
			ComboAudioComps[i]->AdjustVolume(FadeDuration, TargetVolume);
		}
	}
}

void UMusicLayer::HandleRateNote()
{
	SetComboPalier(0, 0.1f);
	SetArmeAudible(false, 0.1f);
	if (PisteActu && PisteActu->FailSound)
	{
		UGameplayStatics::PlaySound2D(this, PisteActu->FailSound, 1.0f);
	}
	if (BaseAudioComp)
	{
		BaseAudioComp->SetLowPassFilterEnabled(true);
		BaseAudioComp->SetLowPassFilterFrequency(250.0f);
	}
	
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetuneTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(DetuneTimerHandle, [this]()
		{
			if (BaseAudioComp)
			{
				BaseAudioComp->SetLowPassFilterEnabled(false);
			}
		}, 0.8f, false);
	}
}

bool UMusicLayer::IsOnBeat(float ToleranceSec) const
{
	if (!GetWorld() || !ClockHandle)
	{
		return false;
	}

	//delegue à GetBeatOffsetMs pour check le timing
	return FMath::Abs(GetBeatOffsetMs()) <= (ToleranceSec * 1000.0f);
}

void UMusicLayer::HandleMetronomeEvent(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	LastBeatWorldTime = World->GetTimeSeconds();
	
	const float DecalageAudio = FMath::Clamp(AudioLatencyOffsetSec, 0.0f, 0.5f);
	if (DecalageAudio > 0.001f)
	{
		//lance le onBeat selon le décalage audio
		World->GetTimerManager().SetTimer(
			BeatLatencyTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this, NumBars, Beat]()
			{
				OnBeat.Broadcast(NumBars, Beat);
			}),
			DecalageAudio,
			false
		);
	}
	else
	{
		OnBeat.Broadcast(NumBars, Beat);
	}
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
