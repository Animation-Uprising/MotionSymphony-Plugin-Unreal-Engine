// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "..\..\Public\MotionMatchingUtil\PoseLookupTable.h"
#include "..\..\Public\MotionMatchingUtil\MotionMatchingUtils.h"

FPoseCandidateSet::FPoseCandidateSet()
{
}

FPoseCandidateSet::FPoseCandidateSet(FPoseMotionData& BasePose, FKMeansClusteringSet& TrajectoryClusters)
	: SetId(BasePose.PoseId)
{
	PoseCandidates.Empty(TrajectoryClusters.Clusters.Num() + 1);

	//Find lowest cost pose from each cluster and add it to this set
	for (FKMCluster& Cluster : TrajectoryClusters.Clusters)
	{
		if (Cluster.Samples.Num() == 0)
			continue;

		float LowestCost = 10000000.0f;
		int32 LowestCostId = -1;
		for (int32 i = 0; i < Cluster.Samples.Num(); ++i)
		{
			if(Cluster.Samples[i]->DoNotUse)
				continue;

			float Cost = FMotionMatchingUtils::ComputePoseCost_SD(BasePose.JointData, Cluster.Samples[i]->JointData, 3.0f, 1.0f);

			Cost += FVector::Distance(BasePose.LocalVelocity, Cluster.Samples[i]->LocalVelocity) * 3.0f;

			if (Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestCostId = i;
			}
		}

		PoseCandidates.Add(Cluster.Samples[LowestCostId]->PoseId);
	}
}

void FPoseCandidateSet::CalculateAveragePose(TArray<FPoseMotionData>& Poses)
{
	AveragePose.Clear();

	int32 CandidateCount = 0;
	for (int32 PoseId : PoseCandidates)
	{
		if (PoseId > -1 && PoseId < Poses.Num())
		{
			AveragePose += Poses[PoseId];
			++CandidateCount;
		}
	}

	AveragePose /= CandidateCount;
}

bool FPoseCandidateSet::CalculateSimilarityAndCombine(FPoseCandidateSet& CompareSet, float CombineTolerance)
{
	TArray<int32> IdsToAddIfCombined;
	IdsToAddIfCombined.Empty(FMath::CeilToInt((float)FMath::Max(CompareSet.PoseCandidates.Num(), PoseCandidates.Num()) * CombineTolerance) + 1);

	int32 SimilarityScore = 0;
	for (int32 CompareCandidateId : CompareSet.PoseCandidates)
	{
		bool Found = false;
		for (int32 CandidateId : PoseCandidates)
		{
			if (CompareCandidateId == CandidateId)
			{
				++SimilarityScore;
				Found = true;
				break;
			}
		}

		if (!Found)
		{
			IdsToAddIfCombined.Add(CompareCandidateId);
		}
	}

	float Similarity = ((float)SimilarityScore * 2.0f) / (float)(CompareSet.PoseCandidates.Num() + PoseCandidates.Num());

	if (Similarity > CombineTolerance)
	{
		for (int32 Id : IdsToAddIfCombined)
		{
			PoseCandidates.Add(Id);
		}

		return true;
	}


	return false;
}

void FPoseCandidateSet::MergeWith(FPoseCandidateSet& MergeSet)
{
	for (int32 MergePoseId : MergeSet.PoseCandidates)
	{
		bool bFound = false;
		for (int32 PoseId : PoseCandidates)
		{
			if (PoseId == MergePoseId)
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			PoseCandidates.Add(MergePoseId);
		}
	}
}

FPoseLookupTable::FPoseLookupTable()
{
}

void FPoseLookupTable::Process(TArray<FPoseMotionData>& Poses, FKMeansClusteringSet& TrajectoryClusters, 
	float CombineThreshold, int32 DesiredLookupTableSize, int32 MaxLookupColumnSize)
{
	CandidateSets.Empty(Poses.Num() + 1);

	//Step 1: Initialize the lookup table with every pose having a column
	//Step 2: For each column add the closest pose from each cluster (see the FPoseCanididateSet constructor)
	for (FPoseMotionData& Pose : Poses)
	{
		CandidateSets.Emplace(FPoseCandidateSet(Pose, TrajectoryClusters));
		CandidateSets.Last().CalculateAveragePose(Poses);
		Pose.CandidateSetId = Pose.PoseId;
	}

	TArray<FRedirectStruct> Redirects;
	Redirects.Empty(100);

	//Step 3: Hierarchically cluster pose lookup lists by their "Average Pose" 
	while (true)
	{
		bool SetsCombined = false;

		float LowestCost = 10000000.0f;
		int32 LowestCostA = -1;
		int32 LowestCostB = -1;
		for (int32 i = 0; i < CandidateSets.Num(); ++i)
		{
			FPoseCandidateSet& KeyCandidateSet = CandidateSets[i];

			if (KeyCandidateSet.PoseCandidates.Num() >= MaxLookupColumnSize)
				continue;

			FPoseMotionData& KeyPoseData = KeyCandidateSet.AveragePose;

			for (int32 k = i + 1; k < CandidateSets.Num(); ++k)
			{
				FPoseCandidateSet& CompareCandidateSet = CandidateSets[k];

				if (CompareCandidateSet.PoseCandidates.Num() >= MaxLookupColumnSize)
					continue;

				float Cost = FMotionMatchingUtils::ComputePoseCost_SD(KeyCandidateSet.AveragePose.JointData, CompareCandidateSet.AveragePose.JointData, 3.0f, 1.0f);

				Cost += FVector::Distance(KeyCandidateSet.AveragePose.LocalVelocity, CompareCandidateSet.AveragePose.LocalVelocity) * 3.0f;

				if (Cost < LowestCost)
				{
					LowestCost = Cost;
					LowestCostA = i;
					LowestCostB = k;
				}
			}
		}

		if (LowestCostA < 0 || LowestCostB < 0)
		{
			//We cannot merge any more columns (probably because each column has reached it's max size
			break;
		}

		FPoseCandidateSet& MergeToSet = CandidateSets[LowestCostA];
		FPoseCandidateSet& MergeFromSet = CandidateSets[LowestCostB];

		//Combine the most similar average poses in the lists
		MergeToSet.MergeWith(MergeFromSet);
		MergeToSet.CalculateAveragePose(Poses);
		FRedirectStruct Redirect;
		Redirect.Id = MergeFromSet.SetId;
		Redirect.RedirectId = MergeToSet.SetId;
		Redirects.Add(Redirect);

		CandidateSets.RemoveAt(LowestCostB);

		if (CandidateSets.Num() <= DesiredLookupTableSize)
		{
			//We've reached the desired size of the lookup table, stop clustering
			break;
		}
	}

	//Step 3: Continuously combine most similar candidate sets until there are K Candidate sets
	//int32 SetsRemaining = CandidateSets.Num();
	//while(true)
	//{
	//	bool SetsCombined = false;
	//	for (int32 i = 0; i < CandidateSets.Num(); ++i)
	//	{
	//		FPoseCandidateSet& KeyCandidateSet = CandidateSets[i];

	//		//DoNotUse Candidate sets should never be the keys of the data set if possible.
	//		if (Poses[KeyCandidateSet.SetId].DoNotUse)
	//			continue;

	//		for (int32 k = i+1; k < CandidateSets.Num(); ++k)
	//		{
	//			FPoseCandidateSet& CompareCandidateSet = CandidateSets[k];

	//			if (KeyCandidateSet.CalculateSimilarityAndCombine(CompareCandidateSet, CombineThreshold))
	//			{
	//				FRedirectStruct Redirect;
	//				Redirect.Id = CompareCandidateSet.SetId;
	//				Redirect.RedirectId = KeyCandidateSet.SetId;
	//				Redirects.Add(Redirect);

	//				CandidateSets.RemoveAt(k);

	//				if(k < i)
	//					--i;

	//				--SetsRemaining;
	//				SetsCombined = true;
	//				break;
	//			}
	//		}
	//	}

	//	if(!SetsCombined)
	//	{
	//		break;
	//	}
	//}

	//Redirect any 'CandidateSetIds' on poses if their candidate list was merged.
	for (FRedirectStruct& Redirect : Redirects)
	{
		Poses[Redirect.Id].CandidateSetId = FindRedirect(Redirect.RedirectId, Redirects);
	}

	//Sort candidate sets by Id in ascending order
	CandidateSets.Sort([](const FPoseCandidateSet& a, const FPoseCandidateSet& b ) { return  a.SetId < b.SetId; });

	//Remap the candidate set Ids to a linear list
	for (FPoseMotionData& Pose : Poses)
	{
		int32 CandidateSetId = Pose.CandidateSetId;

		for (int32 i = 0; i < CandidateSets.Num(); ++i)
		{
			if (Pose.CandidateSetId == CandidateSets[i].SetId)
			{
				Pose.CandidateSetId = i;
			}
		}
	}
}


int FPoseLookupTable::FindRedirect(int32 RedirectId, TArray<FRedirectStruct>& Redirects)
{
	//First search Candidates for the redirect
	for (FPoseCandidateSet& Candidate : CandidateSets)
	{
		if (Candidate.SetId == RedirectId)
		{
			return Candidate.SetId;
		}
	}

	//If it wasn't found, search the redirects for it
	for (FRedirectStruct& Redirect : Redirects)
	{
		if (Redirect.Id == RedirectId)
		{
			return FindRedirect(Redirect.RedirectId, Redirects);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Redirect Failed. Oh Dear... what are we going to do about this?"))
	return -1;
}
