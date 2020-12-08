// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CalibrationData.generated.h"

/** A data structure containing weightins and multipliers for specific motion
matching aspects. Motion Matchign distance costs are multiplied by these 
weights where relevant to calibrate the animation data.*/
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FCalibrationData
{
	GENERATED_USTRUCT_BODY()

public:
	/** Weight of matched joint positions (Default 3.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_Position;

	/** Weight of matched joint velocities (Default 1.0f)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_Velocity;

	/** Weight of match joint resultant velocity (Default 0.1f). The resultant
	velocity is the velocity that would occur if the system switched from the 
	current pose to this one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_ResVelocity;

	/** Weight of the body local velocity (Default 5.0)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float BodyWeight_Velocity;

	/** Weight of the body rotational velocity (Default 3.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float BodyWeight_RotationalVelocity;

	/** Weight of trajectory point positions (Default 12.0)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float TrajectoryWeight_Position;

	/** Weight of trajectory point rotations (Default 6.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float TrajectoryWeight_Rotation;

public:
	FCalibrationData();

public:
	void SetCalibration(float PoseWeightPos, float PoseWeightVel, float PoseWeightRes,
		float BodyWeightVel, float BodyWeightRotVel, float TrajectoryWeightPos, float TrajectoryWeightRot);
};