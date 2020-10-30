// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "..\..\Public\Data\PoseMotionData.h"
#include "..\..\Public\Data\TrajectoryPoint.h"
#include "KMeansClustering.h"
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
	FPoseCandidateSet(FPoseMotionData& BasePose, FKMeansClusteringSet& TrajectoryClusters);

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

	void Process(TArray<FPoseMotionData>& Poses, FKMeansClusteringSet& TrajectoryClusters, 
		float CombineThreshold, int32 DesiredLookupTableSize, int32 MaxLookupColumnSize);

private:
	int FindRedirect(int32 RedirectId, TArray<FRedirectStruct>& Redirects);
};