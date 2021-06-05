// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Chaos/AABB.h"
#include "Chaos/AABB.h"
#include "Data/PoseMotionData.h"
#include "Data/TrajectoryPoint.h"
#include "Math/UnrealMathUtility.h"
#include "KMeansClustering.generated.h"

class UMotionCalibration;
struct FCalibrationData;

USTRUCT()
struct MOTIONSYMPHONY_API FKMCluster
{
	GENERATED_BODY()

public:
	float Variance;
	FColor DebugDrawColor;

	TArray<FPoseMotionData> Samples;
	TArray<FTrajectoryPoint> Center;

public:
	FKMCluster();
	FKMCluster(FPoseMotionData& BasePose, int32 EstimatedSamples);
	
	float ComputePoseCost(FPoseMotionData& Pose, FCalibrationData& Calibration);
	void AddPose(FPoseMotionData& Pose);
	float CalculateVariance();

	float ReCalculateCenter();
	void Reset();
};


USTRUCT()
struct MOTIONSYMPHONY_API FKMeansClusteringSet
{
	GENERATED_BODY()

public:
	int32 K;
	TArray<FKMCluster> Clusters;

	float Variance;

	FCalibrationData* Calibration;

public:
	FKMeansClusteringSet();
	void BeginClustering(TArray<FPoseMotionData>& Poses, FCalibrationData& InCalibration,
	                     const int32 InK, const int32 MaxIterations, const bool bFast = false);
	float CalculateVariance();
	
	void Clear();

private: 
	void InitializeClusters(TArray<FPoseMotionData>& Poses);
	void InitializeClustersFast(TArray<FPoseMotionData>& Poses);
	bool ProcessClusters(TArray<FPoseMotionData>& Poses);
};