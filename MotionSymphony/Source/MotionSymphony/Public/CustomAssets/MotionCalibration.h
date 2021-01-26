// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/CalibrationData.h"
#include "MotionCalibration.generated.h"

class UMotionDataAsset;

UCLASS()
class MOTIONSYMPHONY_API UMotionCalibration : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMotionDataAsset* AssotiatedMotionData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCalibrationData CalibrationData;

public:
	UMotionCalibration(const FObjectInitializer& ObjectInitializer);
};