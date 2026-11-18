#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MusicDataAsset.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class FINALPROJECT_API UMusicDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music Timing", meta = (ClampMin = "40.0", ClampMax = "600.0"))
	float BPM = 130.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music Timing", meta = (ClampMin = "1", ClampMax = "16"))
	int32 BeatsPerBar = 4;

	//piste principale
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TObjectPtr<USoundBase> BasePiste;

	//pistes des combos (index 0 = palier 1, index 1 = palier 2, index 2 = palier 3)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TArray<TObjectPtr<USoundBase>> ComboPistes;

	//piste des armes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Music")
	TMap<FName, TObjectPtr<USoundBase>> ArmePiste;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SoundFX")
	TObjectPtr<USoundBase> FailSound;
};