// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CalibrationData.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FCalibrationData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float PoseWeight_Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float PoseWeight_Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float PoseWeight_ResVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float BodyWeight_Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float TrajectoryWeight_Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
		float TrajectoryWeight_Rotation;

public:
	FCalibrationData();

public:
	void SetCalibration(float a_poseWeightPos, float a_poseWeightVel, float a_poseWeightRes,
		float a_bodyWeightVel, float a_trajectoryWeightPos, float a_trajectoryWeightRot);
};