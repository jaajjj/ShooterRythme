#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FinalProjectGameMode.generated.h"

UCLASS(abstract)
class FINALPROJECT_API AFinalProjectGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFinalProjectGameMode();
	
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void AjouterCombo();

	//remet le combo à 0 et déclenche la fausse note
	UFUNCTION(BlueprintCallable, Category = "Combo")
	void ResetCombo();

	UFUNCTION(BlueprintPure, Category = "Combo")
	int32 GetCombo() const { return Combo; }

protected:
	//seuils de combo pour activer les 3 pistes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier1 = 15;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier2 = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier3 = 50;

private:
	int32 Combo = 0;
	int32 PalierActuel = 0;

	void ActualiserPalierAudio();
};