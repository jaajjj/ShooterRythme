#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HitErrorBarWidget.generated.h"

UCLASS(Abstract)
class FINALPROJECT_API UHitErrorBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Rhythm Math")
	void CalculateHitData(float OffsetMs, float ToleranceMaxMs, float& OutPosX, FLinearColor& OutColor) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Math")
	float HalfBarWidth = 200.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Math", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PerfectRatio = 0.303f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Math", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GoodRatio = 0.638f;
	
	//seuil timing
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Math")
	float PerfectThresholdMs = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rhythm Math")
	float GoodThresholdMs = 70.0f;

	//couleurs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Math")
	FLinearColor PerfectColor = FLinearColor(0.056f, 0.413f, 0.753f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Math")
	FLinearColor GoodColor = FLinearColor(0.144f, 0.68f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm Math")
	FLinearColor BadColor = FLinearColor(0.88f, 0.371f, 0.053f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rhythm|Math")
	FLinearColor MissColor = FLinearColor(1.0f, 0.1f, 0.1f, 1.0f); 
};