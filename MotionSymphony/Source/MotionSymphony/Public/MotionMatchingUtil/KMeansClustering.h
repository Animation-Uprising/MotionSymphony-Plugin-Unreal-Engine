// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\..\Public\Data\PoseMotionData.h"
#include "..\..\Public\Data\TrajectoryPoint.h"
#include "Math/UnrealMathUtility.h"
#include "KMeansClustering.generated.h"



USTRUCT()
struct MOTIONSYMPHONY_API FKMCluster
{
	GENERATED_BODY()

public:
	float Variance;
	FColor DebugDrawColor;

	TArray<FPoseMotionData*> Samples;
	TArray<FTrajectoryPoint> Center;

public:
	FKMCluster();
	FKMCluster(FPoseMotionData& BasePose, int32 EstimatedSamples);
	
	float ComputePoseCost(FPoseMotionData& Pose);
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

public:
	FKMeansClusteringSet();
	void BeginClustering(TArray<FPoseMotionData>& Poses, int32 InK, int32 MaxIterations, bool bFast = false);
	float CalculateVariance();

private: 
	void InitializeClusters(TArray<FPoseMotionData>& Poses);
	void InitializeClustersFast(TArray<FPoseMotionData>& Poses);
	bool ProcessClusters(TArray<FPoseMotionData>& Poses);
};