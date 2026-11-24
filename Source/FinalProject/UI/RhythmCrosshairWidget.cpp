#include "RhythmCrosshairWidget.h"
#include "RhythmTickWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Animation/WidgetAnimation.h"
#include "Character/FinalProjectCharacter.h"
#include "Settings/RhythmSettings.h"
#include "../Logic/Music/MusicLayer.h"

URhythmCrosshairWidget::URhythmCrosshairWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//couleurs par défaut par instrument
	CouleursArmes.Add(TEXT("trumpet"), FLinearColor(1.0f, 0.78f, 0.1f, 1.0f)); 
	CouleursArmes.Add(TEXT("guitar"),  FLinearColor(1.0f, 0.45f, 0.08f, 1.0f)); 
	CouleursArmes.Add(TEXT("piano"),   FLinearColor(0.1f, 0.75f, 1.0f, 1.0f));
	CouleurActuelle = CouleursArmes.FindRef(TEXT("trumpet"));
}

void URhythmCrosshairWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//crée dynamiquement le pool de barres selon NotesVisibles + 1 (pour afficher la note en retard qui traverse)
	EnsurePoolSize(NotesVisibles + 1);
}

void URhythmCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UWorld* World = GetWorld())
	{
		if (UMusicLayer* Music = World->GetSubsystem<UMusicLayer>())
		{
			Music->OnBeat.AddUniqueDynamic(this, &URhythmCrosshairWidget::HandleMusicBeat);
		}
	}

	BindCharacterEvents();
}

void URhythmCrosshairWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		if (UMusicLayer* Music = World->GetSubsystem<UMusicLayer>())
		{
			Music->OnBeat.RemoveDynamic(this, &URhythmCrosshairWidget::HandleMusicBeat);
		}
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AFinalProjectCharacter* Char = Cast<AFinalProjectCharacter>(PC->GetPawn()))
		{
			Char->OnPlayerShoot.RemoveDynamic(this, &URhythmCrosshairWidget::HandlePlayerShoot);
		}
	}

	Super::NativeDestruct();
}

void URhythmCrosshairWidget::BindCharacterEvents()
{
	if (bCharacterBound)
	{
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AFinalProjectCharacter* Char = Cast<AFinalProjectCharacter>(PC->GetPawn()))
		{
			Char->OnPlayerShoot.AddUniqueDynamic(this, &URhythmCrosshairWidget::HandlePlayerShoot);
			bCharacterBound = true;
		}
	}
}

void URhythmCrosshairWidget::PlayBeatPulse()
{
	if (Anim_BeatPulse)
	{
		PlayAnimation(Anim_BeatPulse, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void URhythmCrosshairWidget::PlayHitSuccess()
{
	if (Anim_HitSuccess)
	{
		PlayAnimation(Anim_HitSuccess, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void URhythmCrosshairWidget::PlayHitMiss()
{
	if (Anim_HitMiss)
	{
		PlayAnimation(Anim_HitMiss, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
	}
}

void URhythmCrosshairWidget::HandleMusicBeat(int32 Bar, int32 Beat)
{
	PlayBeatPulse();
}

void URhythmCrosshairWidget::HandlePlayerShoot(float OffsetMs, float ToleranceMaxMs)
{
	if (FMath::Abs(OffsetMs) <= ToleranceMaxMs)
	{
		PlayHitSuccess();
	}
	else
	{
		PlayHitMiss();
	}
}

void URhythmCrosshairWidget::EnsurePoolSize(int32 DesiredSize)
{
	if (!NotesCanvas || !TickWidgetClass)
	{
		return;
	}

	while (PoolTicksGauche.Num() < DesiredSize)
	{
		if (URhythmTickWidget* LeftTick = CreateWidget<URhythmTickWidget>(this, TickWidgetClass))
		{
			if (UCanvasPanelSlot* PanelSlot = NotesCanvas->AddChildToCanvas(LeftTick))
			{
				PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				PanelSlot->SetPosition(FVector2D::ZeroVector);
				PanelSlot->SetAutoSize(true);
			}
			LeftTick->SetVisibility(ESlateVisibility::Collapsed);
			LeftTick->SetTickColor(CouleurActuelle);
			LeftTick->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			LeftTick->SetRenderScale(FVector2D(1.0f, 1.0f));
			PoolTicksGauche.Add(LeftTick);
		}
		else
		{
			break;
		}
	}

	while (PoolTicksDroite.Num() < DesiredSize)
	{
		if (URhythmTickWidget* RightTick = CreateWidget<URhythmTickWidget>(this, TickWidgetClass))
		{
			if (UCanvasPanelSlot* PanelSlot = NotesCanvas->AddChildToCanvas(RightTick))
			{
				PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
				PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				PanelSlot->SetPosition(FVector2D::ZeroVector);
				PanelSlot->SetAutoSize(true);
			}
			RightTick->SetVisibility(ESlateVisibility::Collapsed);
			RightTick->SetTickColor(CouleurActuelle);
			RightTick->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			RightTick->SetRenderScale(FVector2D(-1.0f, 1.0f)); //miroir horizontal (<)
			PoolTicksDroite.Add(RightTick);
		}
		else
		{
			break;
		}
	}
}

void URhythmCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const int32 TotalPool = NotesVisibles + 1;
	EnsurePoolSize(TotalPool);

	if (!bCharacterBound)
	{
		BindCharacterEvents();
	}

	//update la position, l'échelle et la couleur de chaque note dans le pool
	//i = 0 : note du beat qui vient de passer (traverse le récepteur en Late de 0 à -140ms)
	//i >= 1 : notes futures en approche vers le récepteur
	for (int32 i = 0; i < TotalPool; ++i)
	{
		float Distance = 0.0f;
		float Opacity = 0.0f;
		float Scale = 1.0f;
		FLinearColor NoteColor = CouleurActuelle;
		bool bVisible = false;

		CalculateTickFull(i, DistanceMax, NotesVisibles, Distance, Opacity, Scale, NoteColor, bVisible);

		if (bVisible)
		{
			//SetRenderTranslation et RenderScale pour un déplacement GPU pur et fluide
			if (PoolTicksGauche.IsValidIndex(i) && PoolTicksGauche[i])
			{
				PoolTicksGauche[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				PoolTicksGauche[i]->SetRenderTranslation(FVector2D(-Distance, 0.0f));
				PoolTicksGauche[i]->SetRenderScale(FVector2D(Scale, Scale));
				PoolTicksGauche[i]->SetTickOpacity(Opacity);
				PoolTicksGauche[i]->SetTickColor(NoteColor);
			}

			if (PoolTicksDroite.IsValidIndex(i) && PoolTicksDroite[i])
			{
				PoolTicksDroite[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				PoolTicksDroite[i]->SetRenderTranslation(FVector2D(Distance, 0.0f));
				PoolTicksDroite[i]->SetRenderScale(FVector2D(-Scale, Scale)); //miroir horizontal (<) + échelle
				PoolTicksDroite[i]->SetTickOpacity(Opacity);
				PoolTicksDroite[i]->SetTickColor(NoteColor);
			}
		}
		else
		{
			if (PoolTicksGauche.IsValidIndex(i) && PoolTicksGauche[i])
			{
				PoolTicksGauche[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (PoolTicksDroite.IsValidIndex(i) && PoolTicksDroite[i])
			{
				PoolTicksDroite[i]->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void URhythmCrosshairWidget::CalculateTickData(int32 NoteIndex, float InDistanceMax, int32 InNotesVisibles, float& OutDistance, float& OutOpacity, bool& bOutVisible) const
{
	float UnusedScale = 1.0f;
	FLinearColor UnusedColor = CouleurActuelle;
	CalculateTickFull(NoteIndex, InDistanceMax, InNotesVisibles, OutDistance, OutOpacity, UnusedScale, UnusedColor, bOutVisible);
}

void URhythmCrosshairWidget::CalculateTickFull(int32 NoteIndex, float InDistanceMax, int32 InNotesVisibles, float& OutDistance, float& OutOpacity, float& OutScale, FLinearColor& OutColor, bool& bOutVisible) const
{
	OutDistance = 0.0f;
	OutOpacity = 0.0f;
	OutScale = 1.0f;
	OutColor = CouleurActuelle;
	bOutVisible = false;

	UWorld* World = GetWorld();
	if (!World)
	{ return; }
	
	//recup le musicLayer 
	UMusicLayer* Music = World->GetSubsystem<UMusicLayer>();
	if (!Music)
	{ return; }

	const float BeatDuration = Music->GetBeatDurationSeconds();
	if (BeatDuration <= 0.001f)
	{ return; }

	const float SafeDistanceMax = (InDistanceMax > 0.0f) ? InDistanceMax : DistanceMax;
	const int32 SafeNotesVisibles = (InNotesVisibles > 0) ? InNotesVisibles : NotesVisibles;

	//délègue à MusicLayer pour obtenir le temps écoulé depuis le dernier beat
	const float TimeSinceLastBeat = Music->GetTimeSinceLastBeat();
	const float TotalTravelTime = BeatDuration * SafeNotesVisibles;

	//temps restant avant l'impact au récepteur (0 = impact exact, négatif = en retard / après le beat)
	const float TimeUntilHit = (BeatDuration * NoteIndex) - TimeSinceLastBeat;

	//récupère la tolérance globale depuis les paramètres du projet (URhythmSettings)
	float LateToleranceSec = 0.12f;
	if (const URhythmSettings* Settings = GetDefault<URhythmSettings>())
	{
		LateToleranceSec = Settings->BeatToleranceSec;
	}
	const float MaxDisplayLateSec = LateToleranceSec + 0.03f; // marge douce pour le fondu de dissolution

	if (TimeUntilHit >= -MaxDisplayLateSec && TimeUntilHit <= TotalTravelTime)
	{
		bOutVisible = true;

		const float SafeImpactDist = FMath::Clamp(ImpactDistance, 0.0f, SafeDistanceMax - 10.0f);
		const float TravelSpeed = (SafeDistanceMax - SafeImpactDist) / TotalTravelTime;

		if (TimeUntilHit >= 0.0f)
		{
			//phase d'approche normale vers le récepteur
			const float Progress = 1.0f - (TimeUntilHit / TotalTravelTime);
			OutDistance = SafeImpactDist + FMath::Max(0.0f, (1.0f - Progress) * (SafeDistanceMax - SafeImpactDist));

			//animation d'entrée douce : échelle 0.75 -> 1.0 et fondu d'opacité
			if (Progress < 0.20f)
			{
				const float SpawnRatio = Progress / 0.20f;
				OutOpacity = FMath::SmoothStep(0.0f, 1.0f, SpawnRatio);
				OutScale = FMath::Lerp(0.75f, 1.0f, SpawnRatio);
			}
			else
			{
				OutOpacity = 1.0f;
				OutScale = 1.0f;
			}

			OutColor = CouleurActuelle;
		}
		else
		{
			//phase de dépassement ("Late" / en retard) : la note traverse le récepteur vers le centre
			const float LateTime = -TimeUntilHit;
			OutDistance = FMath::Max(20.0f, SafeImpactDist - (TravelSpeed * LateTime));

			//l'échelle rétrécit progressivement pour montrer que la note s'épuise (1.0 -> 0.75)
			OutScale = FMath::Lerp(1.0f, 0.75f, FMath::Clamp(LateTime / LateToleranceSec, 0.0f, 1.0f));

			//effet braise/extinction universel : assombrissement, perte d'éclat et nuance rouge avertissement
			const float LateFactor = FMath::Clamp(LateTime / LateToleranceSec, 0.0f, 1.0f);
			const float Luminance = (CouleurActuelle.R * 0.3f) + (CouleurActuelle.G * 0.59f) + (CouleurActuelle.B * 0.11f);
			const FLinearColor DimmedColor = FLinearColor(Luminance * 0.4f, Luminance * 0.4f, Luminance * 0.4f, CouleurActuelle.A);
			const FLinearColor WarningRed = FLinearColor(0.85f, 0.15f, 0.1f, CouleurActuelle.A);
			const FLinearColor BurnoutColor = FMath::Lerp(DimmedColor, WarningRed, 0.35f);

			OutColor = FMath::Lerp(CouleurActuelle, BurnoutColor, LateFactor);

			//opacité : reste bien visible pendant la fenêtre de tolérance puis se dissout
			if (LateTime <= LateToleranceSec)
			{
				OutOpacity = FMath::Lerp(1.0f, 0.85f, LateTime / LateToleranceSec);
			}
			else
			{
				//dissolution rapide sur les 30 dernières ms
				const float DissolveRatio = (LateTime - LateToleranceSec) / 0.03f;
				OutOpacity = FMath::Max(0.0f, 0.85f * (1.0f - DissolveRatio));
			}
		}
	}
}

FLinearColor URhythmCrosshairWidget::GetCouleurInstrument(FName ArmeID) const
{
	if (const FLinearColor* Trouve = CouleursArmes.Find(ArmeID))
	{
		return *Trouve;
	}
	return CouleurParDefaut;
}

void URhythmCrosshairWidget::ChangerCouleurInstrument(FName ArmeID)
{
	CouleurActuelle = GetCouleurInstrument(ArmeID);

	//applique la couleur à tous les ticks du pool
	for (URhythmTickWidget* Tick : PoolTicksGauche)
	{
		if (Tick) Tick->SetTickColor(CouleurActuelle);
	}
	for (URhythmTickWidget* Tick : PoolTicksDroite)
	{
		if (Tick) Tick->SetTickColor(CouleurActuelle);
	}
}
