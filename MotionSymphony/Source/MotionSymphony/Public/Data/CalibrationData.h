//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Objects/Assets/MotionCalibration.h"
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
	TArray<float> Weights;

public:
	FCalibrationData();
	FCalibrationData(UMotionDataAsset* SourceMotionData);
	FCalibrationData(UMotionMatchConfig* SourceConfig);
	FCalibrationData(const int32 AtomCount);

	void Initialize(const int32 AtomCount);
	void Initialize(UMotionMatchConfig* SourceConfig);
	bool IsValidWithConfig(const UMotionMatchConfig* MotionConfig);

	void GenerateStandardDeviationWeights(const UMotionDataAsset* SourceMotionData, const FMotionTraitField& MotionTrait);
	void GenerateStandardDeviationWeights(const TArray<float>& PoseMatrix, UMotionMatchConfig* InMMConfig);
	void GenerateFinalWeights(const UMotionCalibration* UserCalibration, const FCalibrationData& StdDeviationNormalizers);
};