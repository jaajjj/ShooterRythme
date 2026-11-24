#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RhythmSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Rythme & Audio"))
class FINALPROJECT_API URhythmSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//décalage audio (en secondes)
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "0.5", UIMax = "0.5"))
	float AudioLatencyOffsetSec = 0.055f;

	//tolérance de frappe du beat (en secondes, ex: 0.12s = +-120ms)
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.01", UIMin = "0.01", ClampMax = "0.5", UIMax = "0.5"))
	float BeatToleranceSec = 0.12f;
};