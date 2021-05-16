// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CustomAssets/MotionCalibration.h"
#include "CalibrationData.generated.h"

struct FJointWeightSet;
struct FTrajectoryWeightSet;
struct FMotionTraitField;
class UMotionDataAsset;
class UMotionMatchConfig;
class UMotionCalibration;

/** A data structure containing weightins and multipliers for specific motion
matching aspects. Motion Matchign distance costs are multiplied by these 
weights where relevant to calibrate the animation data.*/
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FCalibrationData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	float Weight_Momentum;

	UPROPERTY()
	float Weight_AngularMomentum;

	UPROPERTY();
	TArray<FJointWeightSet> PoseJointWeights;

	UPROPERTY();
	TArray<FTrajectoryWeightSet> TrajectoryWeights;

public:
	FCalibrationData();
	FCalibrationData(UMotionDataAsset* SourceMotionData);
	FCalibrationData(UMotionMatchConfig* SourceConfig);
	FCalibrationData(int32 PoseJointCount, int32 TrajectoryPointCount);

	void Initialize(UMotionMatchConfig* SourceConfig);
	bool IsValidWithConfig(UMotionMatchConfig* MotionConfig);

	void GenerateStandardDeviationWeights(const UMotionDataAsset* SourceMotionData, const FMotionTraitField& MotionTrait);
	void GenerateFinalWeights(const UMotionCalibration* UserCalibration, const FCalibrationData& StdDeviationNormalizers);
};