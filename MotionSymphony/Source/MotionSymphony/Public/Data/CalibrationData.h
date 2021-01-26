// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CalibrationData.generated.h"

/** A data structure containing wieghtings for a single pose element (i.e. joint). This includes
botht he position and velocity weight of the joint.*/
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FPoseWeightSet
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration");
	float PositionWeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration");
	float VelocityWeight;

	FPoseWeightSet();

	FPoseWeightSet(float PosWeight, float VelWeight);
};


/** A data structure containing weightins and multipliers for specific motion
matching aspects. Motion Matchign distance costs are multiplied by these 
weights where relevant to calibrate the animation data.*/
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FCalibrationData
{
	GENERATED_USTRUCT_BODY()

public:
	/** */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration", meta = (ClampMin=0, ClampMax=1))
	float PoseTrajectoryBalance;

	/** Weight of the body local velocity (Default 5.0)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float BodyWeight_Velocity;

	/** Weight of the body rotational velocity (Default 3.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float BodyWeight_RotationalVelocity;
	
	/** Weight of matched joint positions (Default 3.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_Position;

	/** Weight of matched joint positions (Default 3.0f) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_Velocity;

	/** Weight of match joint resultant velocity (Default 0.1f). The resultant
	velocity is the velocity that would occur if the system switched from the 
	current pose to this one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float PoseWeight_ResVelocity;

	/** Weight of trajectory point positions (Default 12.0)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float TrajectoryWeight_Position;

	/** Weight of trajectory point rotations (Default 6.0) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration")
	float TrajectoryWeight_Rotation;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Calibration");
	TArray<FPoseWeightSet> PoseJointWeights;

public:
	FCalibrationData();
	FCalibrationData(class UMotionDataAsset* SourceMotionData);
	FCalibrationData(class UMotionMatchConfig* SourceConfig);
	FCalibrationData(int32 PoseJointCount);

public:
	void SetCalibration(float PoseTrajBalance, float PoseWeightPos, float PoseWeightVel, float PoseWeightRes,
		float BodyWeightVel, float BodyWeightRotVel, float TrajectoryWeightPos, float TrajectoryWeightRot);
};