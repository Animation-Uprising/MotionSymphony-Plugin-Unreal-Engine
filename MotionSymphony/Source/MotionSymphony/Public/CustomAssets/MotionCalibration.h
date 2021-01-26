// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FCalibrationData;

UCLASS()
class MOTIONSYMPHONY_API UMotionCalibration : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	MotionD

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCalibrationData CalibrationData;

public:
	UMotionCalibration(const FObjectInitializer& ObjectInitializer);
};