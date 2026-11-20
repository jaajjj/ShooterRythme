#include "RhythmCrosshairWidget.h"
#include "RhythmTickWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "../Logic/Music/MusicLayer.h"

URhythmCrosshairWidget::URhythmCrosshairWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//couleurs par défaut par instrument
	CouleursArmes.Add(TEXT("trumpet"), FLinearColor(1.0f, 0.78f, 0.1f, 1.0f)); // Or
	CouleursArmes.Add(TEXT("guitar"),  FLinearColor(1.0f, 0.45f, 0.08f, 1.0f)); // Orange
	CouleursArmes.Add(TEXT("piano"),   FLinearColor(0.1f, 0.75f, 1.0f, 1.0f)); // Cyan
	CouleurActuelle = CouleursArmes.FindRef(TEXT("trumpet"));
}

void URhythmCrosshairWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	//crée dynamiquement le pool de barres selon NotesVisibles
	if (NotesCanvas && TickWidgetClass)
	{
		//on crée les tick de chaques coté
		for (int32 i = 0; i < NotesVisibles; ++i)
		{
			//barre gauche
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
				PoolTicksGauche.Add(LeftTick);
			}

			//barre droite
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
				PoolTicksDroite.Add(RightTick);
			}
		}
	}
}

void URhythmCrosshairWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	//updatela position de chaque note dans le pool à partir du calcul
	for (int32 i = 0; i < NotesVisibles; ++i)
	{
		float Distance = 0.0f;
		float Opacity = 0.0f;
		bool bVisible = false;

		CalculateTickData(i, DistanceMax, NotesVisibles, Distance, Opacity, bVisible);

		if (bVisible)
		{
			//SetRenderTranslation pour faire glisser les note
			if (PoolTicksGauche.IsValidIndex(i) && PoolTicksGauche[i])
			{
				PoolTicksGauche[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				PoolTicksGauche[i]->SetRenderTranslation(FVector2D(-Distance, 0.0f));
				PoolTicksGauche[i]->SetTickOpacity(Opacity);
			}

			if (PoolTicksDroite.IsValidIndex(i) && PoolTicksDroite[i])
			{
				PoolTicksDroite[i]->SetVisibility(ESlateVisibility::HitTestInvisible);
				PoolTicksDroite[i]->SetRenderTranslation(FVector2D(Distance, 0.0f));
				PoolTicksDroite[i]->SetTickOpacity(Opacity);
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
	OutDistance = 0.0f;
	OutOpacity = 0.0f;
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

	//temps restant avant l'impact au centre (0 = impact exact)
	const float TimeUntilHit = (BeatDuration * (NoteIndex + 1)) - TimeSinceLastBeat;

	//visible entre son apparition et 80ms après le passage du centre
	if (TimeUntilHit >= -0.08f && TimeUntilHit <= TotalTravelTime)
	{
		bOutVisible = true;

		//progression normalisée : 0.0 à l'apparition -> 1.0 au centre exact
		const float Progress = 1.0f - (TimeUntilHit / TotalTravelTime);
		OutDistance = FMath::Max(0.0f, (1.0f - Progress) * SafeDistanceMax);

		//fondu d'apparition doux et disparition rapide après le centre
		if (Progress < 0.25f)
		{
			OutOpacity = Progress / 0.25f;
		}
		else if (TimeUntilHit < 0.0f)
		{
			OutOpacity = FMath::Max(0.0f, 1.0f - (FMath::Abs(TimeUntilHit) / 0.08f));
		}
		else
		{
			OutOpacity = 1.0f;
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
