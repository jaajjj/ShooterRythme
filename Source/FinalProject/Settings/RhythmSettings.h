#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RhythmSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Rythme & Audio"))
class FINALPROJECT_API URhythmSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//décalage audio
	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category = "Timing", meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "0.5", UIMax = "0.5"))
	float AudioLatencyOffsetSec = 0.055f;
};