// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "PoseLookupTable.h"
#include "CustomAssets/MotionCalibration.h"
#include "Data/CalibrationData.h"
#include "MotionMatchingUtil\MotionMatchingUtils.h"
#include "CustomAssets/MotionDataAsset.h"

FPoseCandidateSet::FPoseCandidateSet()
	: SetId(-1)
{
}

FPoseCandidateSet::FPoseCandidateSet(FPoseMotionData& BasePose, 
	FKMeansClusteringSet& TrajectoryClusters, FCalibrationData& InCalibration)
	: SetId(BasePose.PoseId)
{
	AveragePose = BasePose;

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
			if(Cluster.Samples[i].bDoNotUse) //Don't add 'DoNotUse' poses to any given Lookup table
			{
				continue;
			}

			// Pose Joint Cost
			float Cost = FMotionMatchingUtils::ComputePoseCost(BasePose.JointData, 
				Cluster.Samples[i].JointData, InCalibration);
				
			//Body Velocity Cost
			Cost += FVector::Distance(BasePose.LocalVelocity, Cluster.Samples[i].LocalVelocity) 
				* InCalibration.Weight_Momentum;

			//Body Rotational Velocity Cost
			Cost += FMath::Abs(BasePose.RotationalVelocity - Cluster.Samples[i].RotationalVelocity) 
				* InCalibration.Weight_AngularMomentum;

			//Pose Favour
			Cost *= Cluster.Samples[i].Favour;

			if (Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestCostId = i;
			}
		}

		PoseCandidates.Add(Cluster.Samples[LowestCostId]);
	}
}

float FPoseCandidateSet::CalculateAveragePose()
{
	FPoseMotionData OldAveragePose = AveragePose;

	AveragePose.Clear();

	int32 CandidateCount = 0;
	for (FPoseMotionData& Pose : PoseCandidates)
	{
		AveragePose += Pose;
		++CandidateCount;
	}

	AveragePose /= CandidateCount;

	float AveragePoseDelta = FMotionMatchingUtils::ComputePoseCost(AveragePose.JointData, OldAveragePose.JointData, 1.0f, 1.0f);

	return AveragePoseDelta;
}

bool FPoseCandidateSet::CalculateSimilarityAndCombine(FPoseCandidateSet& CompareSet, float CombineTolerance)
{
	TArray<FPoseMotionData> PosesToAddIfCombined;
	PosesToAddIfCombined.Empty(FMath::CeilToInt((float)FMath::Max(CompareSet.PoseCandidates.Num(), PoseCandidates.Num()) * CombineTolerance) + 1);

	int32 SimilarityScore = 0;
	for (FPoseMotionData& CompareCandidate : CompareSet.PoseCandidates)
	{
		bool Found = false;
		for (FPoseMotionData& Candidate : PoseCandidates)
		{
			if (CompareCandidate.PoseId == Candidate.PoseId)
			{
				++SimilarityScore;
				Found = true;
				break;
			}
		}

		if (!Found)
		{
			PosesToAddIfCombined.Add(CompareCandidate);
		}
	}

	float Similarity = ((float)SimilarityScore * 2.0f) / (float)(CompareSet.PoseCandidates.Num() + PoseCandidates.Num());

	if (Similarity > CombineTolerance)
	{
		for (FPoseMotionData& Pose : PosesToAddIfCombined)
		{
			PoseCandidates.Add(Pose);
		}

		return true;
	}


	return false;
}

void FPoseCandidateSet::MergeWith(FPoseCandidateSet& MergeSet)
{
	for (FPoseMotionData& MergePose : MergeSet.PoseCandidates)
	{
		bool bFound = false;
		for (FPoseMotionData& Pose : PoseCandidates)
		{
			if (Pose.PoseId == MergePose.PoseId)
			{
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			MergePose.CandidateSetId = SetId;
			PoseCandidates.Add(MergePose);
		}
	}
}

void FPoseCandidateSet::SerializeCandidatePoseIds()
{
	PoseCandidateIds.Empty(PoseCandidates.Num() + 1);

	for (FPoseMotionData& Pose : PoseCandidates)
	{
		PoseCandidateIds.Add(Pose.PoseId);
	}

	PoseCandidates.Empty(PoseCandidates.Num() + 1);
}

void FPoseCandidateSet::InitializeRuntime(TArray<FPoseMotionData>& FullPoseList)
{
	PoseCandidates.Empty(PoseCandidateIds.Num() + 1);
	for (const int32 PoseId : PoseCandidateIds)
	{
		PoseCandidates.Add(FullPoseList[FMath::Clamp(PoseId, 0, FullPoseList.Num() - 1)]);
	}
}

FPoseLookupTable::FPoseLookupTable()
{}

void FPoseLookupTable::Process(TArray<FPoseMotionData>& Poses, FKMeansClusteringSet& TrajectoryClusters, 
	FCalibrationData& InCalibration, const int32 DesiredLookupTableSize)
{
	CandidateSets.Empty(Poses.Num() + 1);

	//Step 1: Initialize the lookup table with every pose having its own column
	//Step 2: For each column add the closest pose from each cluster (see the FPoseCanididateSet constructor)
	TArray<FPoseCandidateSet> CandidateSetSamples;
	for (FPoseMotionData& Pose : Poses)
	{
		/*CandidateSets.Emplace(FPoseCandidateSet(Pose, TrajectoryClusters, InCalibration));
		CandidateSets.Last().CalculateAveragePose();
		Pose.CandidateSetId = Pose.PoseId;*/

		CandidateSetSamples.Emplace(FPoseCandidateSet(Pose, TrajectoryClusters, InCalibration));
		CandidateSetSamples.Last().CalculateAveragePose();
	}

	//Step 3: Initialize KMeans starting clusters for clustering the lists based on pose.
	TArray<FPoseCandidateSet*> CandidateSetSamplesCopy;
	CandidateSetSamplesCopy.Empty(CandidateSetSamples.Num() + 1);
	for (FPoseCandidateSet& CandidateSet : CandidateSetSamples)
	{
		CandidateSetSamplesCopy.Add(&CandidateSet);
	}

	int32 RandomStartingCluster = FMath::RandRange(0, CandidateSetSamples.Num() - 1);
	CandidateSets.Add(CandidateSetSamples[RandomStartingCluster]);
	CandidateSetSamplesCopy.RemoveAt(RandomStartingCluster);

	int32 Iterations = FMath::Min(CandidateSetSamplesCopy.Num(), DesiredLookupTableSize);
	for (int32 i = 1; i < Iterations; ++i)
	{
		float HighestSetCost = -20000000.0f;
		int32 HighestSetId = 0;
		for (int32 k = 0; k < CandidateSetSamplesCopy.Num(); ++k)
		{
			float LowestSetCost = 20000000.0f;
			for(int32 j = 0; j < CandidateSets.Num(); ++j)
			{
				const float Cost = FMotionMatchingUtils::ComputePoseCost(CandidateSets[j].AveragePose.JointData,
					CandidateSetSamplesCopy[k]->AveragePose.JointData, InCalibration);

				if (Cost < LowestSetCost)
				{
					LowestSetCost = Cost;
				}
			}

			if (LowestSetCost > HighestSetCost)
			{
				HighestSetCost = LowestSetCost;
				HighestSetId = k;
			}
		}

		CandidateSets.Add(*CandidateSetSamplesCopy[HighestSetId]);
		CandidateSetSamplesCopy.RemoveAt(HighestSetId);

		if(CandidateSetSamplesCopy.Num() == 0)
		{
			break;
		}
	}

	//Step 4: Continuously process the clusters until Max Iterations (10) is reached or until the clusters no longer change
	for (int32 i = 0; i < 10; ++i)
	{
		//Empty all the clusters but retain their average Pose
		for (FPoseCandidateSet& Cluster : CandidateSets)
		{
			Cluster.PoseCandidates.Empty(Cluster.PoseCandidates.Num() + 1);
		}

		int32 NumClusters = CandidateSets.Num();

		for (FPoseCandidateSet& CandidateSetSample : CandidateSetSamples)
		{
			//Cost Function to find the best cluster for this set to fit in
			float LowestClusterCost = 1000000.0f;
			int32 LowestClusterId = -1;
			for (int32 k = 0; k < NumClusters; ++k)
			{
				float Cost = FMotionMatchingUtils::ComputePoseCost(CandidateSetSample.AveragePose.JointData, CandidateSets[k].AveragePose.JointData, InCalibration);

				if (Cost < LowestClusterCost)
				{
					LowestClusterCost = Cost;
					LowestClusterId = k;
				}
			}

			//Add the Candidate set to the closest cluster
			CandidateSets[LowestClusterId].MergeWith(CandidateSetSample);
		}

		bool bClustersChanged = false;
		float ClusterDeltaTolerance = 1.0f;
		for (FPoseCandidateSet& Cluster : CandidateSets)
		{
			if (Cluster.CalculateAveragePose() > ClusterDeltaTolerance)
			{
				bClustersChanged = true;
			}
		}

		if(!bClustersChanged)
		{
			break;
		}
	}


	for (int32 i = 0; i < CandidateSets.Num(); ++i)
	{
		if (CandidateSets[i].PoseCandidates.Num() == 0)
		{
			CandidateSets.RemoveAt(i);
			--i;
		}
	}



	//Step 3: Hierarchically cluster pose lookup lists by their "Average Pose" 
	//while (true)
	//{
	//	bool SetsCombined = false;

	//	float LowestCost = 10000000.0f;
	//	int32 LowestCostA = -1;
	//	int32 LowestCostB = -1;
	//	for (int32 i = 0; i < CandidateSets.Num(); ++i)
	//	{
	//		FPoseCandidateSet& KeyCandidateSet = CandidateSets[i];

	//		if (KeyCandidateSet.PoseCandidates.Num() >= MaxLookupColumnSize)
	//			continue;

	//		FPoseMotionData& KeyPoseData = KeyCandidateSet.AveragePose;

	//		for (int32 k = i + 1; k < CandidateSets.Num(); ++k)
	//		{
	//			FPoseCandidateSet& CompareCandidateSet = CandidateSets[k];

	//			if (CompareCandidateSet.PoseCandidates.Num() >= MaxLookupColumnSize)
	//				continue;

	//			FPoseMotionData& ComparePoseData = CompareCandidateSet.AveragePose;

	//			//Pose Joint Cost
	//			float Cost = FMotionMatchingUtils::ComputePoseCost_SD(KeyPoseData.JointData, 
	//				ComparePoseData.JointData, InCalibration);

	//			//Body Velocity Cost
	//			Cost += FVector::Distance(KeyPoseData.LocalVelocity, ComparePoseData.LocalVelocity) 
	//				* InCalibration.Weight_Momentum;

	//			//Body Rotational Velocity Cost
	//			Cost += FMath::Abs(KeyPoseData.RotationalVelocity - ComparePoseData.RotationalVelocity) 
	//				* InCalibration.Weight_AngularMomentum;

	//			if (Cost < LowestCost)
	//			{
	//				LowestCost = Cost;
	//				LowestCostA = i;
	//				LowestCostB = k;
	//			}
	//		}
	//	}

	//	if (LowestCostA < 0 || LowestCostB < 0)
	//	{
	//		//We cannot merge any more columns (probably because each column has reached it's max size
	//		break;
	//	}

	//	FPoseCandidateSet& MergeToSet = CandidateSets[LowestCostA];
	//	FPoseCandidateSet& MergeFromSet = CandidateSets[LowestCostB];

	//	//Combine the most similar average poses in the lists
	//	MergeToSet.MergeWith(MergeFromSet);
	//	MergeToSet.CalculateAveragePose();

	//	CandidateSets.RemoveAt(LowestCostB);

	//	if (CandidateSets.Num() <= DesiredLookupTableSize)
	//	{
	//		//We've reached the desired size of the lookup table, stop clustering
	//		break;
	//	}
	//}


	/*for (int32 i = 0; i < CandidateSets.Num(); ++i)
	{
		FPoseCandidateSet& CandidateSet = CandidateSets[i];
		CandidateSet.SetId = i;

		for (FPoseMotionData& Pose : CandidateSet.PoseCandidates)
		{
			Pose.CandidateSetId = i;
			Poses[Pose.PoseId].CandidateSetId = i;
		}
	}*/
}

void FPoseLookupTable::InitializeRuntime(UMotionDataAsset* MotionDataAsset)
{
	for (FPoseCandidateSet& CandidateSet : CandidateSets)
	{
		CandidateSet.InitializeRuntime(MotionDataAsset->Poses);
	}
}