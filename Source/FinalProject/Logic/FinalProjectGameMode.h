#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FinalProjectGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnComboChange, int32, Pallier, int32, NewCombo);

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
	
	UPROPERTY(BlueprintAssignable, Category = "Combo")
	FOnComboChange OnComboChange;

protected:
	//seuil de combo pour activer la piste instrument de l'arme
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilArme = 5;

	//seuils de combo pour activer les 3 pistes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier1 = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier2 = 25;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	int32 SeuilPalier3 = 60;

private:
	int32 Combo = 0;
	int32 PalierActuel = 0;

	void ActualiserPalierAudio();
};