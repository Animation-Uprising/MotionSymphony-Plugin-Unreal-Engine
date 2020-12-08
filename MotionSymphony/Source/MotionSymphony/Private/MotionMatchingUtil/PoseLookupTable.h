// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PoseMotionData.h"
#include "Data/TrajectoryPoint.h"
#include "KMeansClustering.h"
#include "Data/CalibrationData.h"
#include "Math/UnrealMathUtility.h"
#include "PoseLookupTable.generated.h"

USTRUCT()
struct MOTIONSYMPHONY_API FPoseCandidateSet
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPoseMotionData AveragePose;

	UPROPERTY()
	TArray<int32> PoseCandidates;

	int32 SetId;

public:
	FPoseCandidateSet();
	FPoseCandidateSet(FPoseMotionData& BasePose, FKMeansClusteringSet& TrajectoryClusters, 
		const FCalibrationData& InCalibration);

	bool CalculateSimilarityAndCombine(FPoseCandidateSet& CompareSet, float CombineTolerance);

	void CalculateAveragePose(TArray<FPoseMotionData>& Poses);
	void MergeWith(FPoseCandidateSet& MergeSet);
};

USTRUCT()
struct MOTIONSYMPHONY_API FPoseLookupTable
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FPoseCandidateSet> CandidateSets;

	struct FRedirectStruct
	{
		int32 Id;
		int32 RedirectId;
	};


public:
	FPoseLookupTable();

	void Process(TArray<FPoseMotionData>& Poses, FKMeansClusteringSet& TrajectoryClusters, const FCalibrationData& InCalibration,
		 const int32 DesiredLookupTableSize, const int32 MaxLookupColumnSize);

private:
	int FindRedirect(int32 RedirectId, TArray<FRedirectStruct>& Redirects);
};