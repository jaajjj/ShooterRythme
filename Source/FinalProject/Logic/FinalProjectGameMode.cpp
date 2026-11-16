#include "FinalProjectGameMode.h"
#include "Music/MusicLayer.h"
#include "Engine/World.h"

AFinalProjectGameMode::AFinalProjectGameMode()
{
}

void AFinalProjectGameMode::AjouterCombo()
{
	++Combo;
	ActualiserPalierAudio();
}

void AFinalProjectGameMode::ResetCombo()
{
	Combo = 0;
	PalierActuel = 0;

	if (UMusicLayer* Music = GetWorld()->GetSubsystem<UMusicLayer>())
	{
		Music->HandleRateNote();
	}
}

void AFinalProjectGameMode::ActualiserPalierAudio()
{
	int32 NouveauPalier = 0;
	if (Combo >= SeuilPalier3)
	{
		NouveauPalier = 3;
	}
	else if (Combo >= SeuilPalier2)
	{
		NouveauPalier = 2;
	}
	else if (Combo >= SeuilPalier1)
	{
		NouveauPalier = 1;
	}

	//ne déclenche le fondu que si on change réellement de palier
	if (NouveauPalier != PalierActuel)
	{
		PalierActuel = NouveauPalier;
		if (UMusicLayer* Music = GetWorld()->GetSubsystem<UMusicLayer>())
		{
			Music->SetComboPalier(PalierActuel, 0.25f);
		}
	}
}