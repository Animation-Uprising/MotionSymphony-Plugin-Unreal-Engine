// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PoseMotionData.h"
#include "Data/TrajectoryPoint.h"
#include "KMeansClustering.h"
#include "Math/UnrealMathUtility.h"
#include "PoseLookupTable.generated.h"

class UMotionCalibration;
struct FCalibrationData;
class UMotionDataAsset;

USTRUCT()
struct MOTIONSYMPHONY_API FPoseCandidateSet
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPoseMotionData AveragePose;

	UPROPERTY()
	int32 SetId;

	UPROPERTY()
	TArray<int32> PoseCandidateIds;

	TArray<FPoseMotionData> PoseCandidates;

public:
	FPoseCandidateSet();
	FPoseCandidateSet(FPoseMotionData& BasePose, FKMeansClusteringSet& TrajectoryClusters, 
		 FCalibrationData& InCalibration);

	bool CalculateSimilarityAndCombine(FPoseCandidateSet& CompareSet, float CombineTolerance);

	float CalculateAveragePose();
	void MergeWith(FPoseCandidateSet& MergeSet);
	void SerializeCandidatePoseIds();

	void InitializeRuntime(TArray<FPoseMotionData>& FullPoseList);
};

USTRUCT()
struct MOTIONSYMPHONY_API FPoseLookupTable
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPoseCandidateSet> CandidateSets;

public:
	FPoseLookupTable();

	void Process(TArray<FPoseMotionData>& Poses, FKMeansClusteringSet& TrajectoryClusters, FCalibrationData& InCalibration,
		 const int32 DesiredLookupTableSize);

	void InitializeRuntime(UMotionDataAsset* MotionDataAsset);
};