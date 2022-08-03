// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MMOptimisation_MultiClustering.h"
#include "CustomAssets/MotionDataAsset.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"

UMMOptimisation_MultiClustering::UMMOptimisation_MultiClustering(const FObjectInitializer& ObjectInitializer)
	: UMMOptimisationModule(ObjectInitializer),
	KMeansClusterCount(100),
	KMeansMaxIterations(10),
	DesiredLookupTableSize(100)
{
}

void UMMOptimisation_MultiClustering::BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset)
{
	Super::BuildOptimisationStructures(InMotionDataAsset);

	PoseLookupSets.Empty();

	//First create trait bins with which to cluster on.
	TMap<FMotionTraitField, TArray<FPoseMotionData> > PoseBins;

	for (FPoseMotionData& Pose : InMotionDataAsset->Poses)
	{
		TArray<FPoseMotionData>& PoseBin = PoseBins.FindOrAdd(Pose.Traits);
		PoseBin.Add(FPoseMotionData(Pose));
	}

	//For each trait bin we need to cluster and create a lookup table
	for (auto& TraitPoseSet : PoseBins)
	{
		FCalibrationData FinalPreProcessCalibration = FCalibrationData();
		FinalPreProcessCalibration.GenerateFinalWeights(InMotionDataAsset->PreprocessCalibration, 
			InMotionDataAsset->FeatureStandardDeviations[TraitPoseSet.Key]);

#if WITH_EDITORONLY_DATA
		KMeansClusteringSet.Clear();
#else
		FKMeansClusteringSet KMeansClusteringSet = FKMeansClusteringSet();
#endif
		KMeansClusteringSet.BeginClustering(TraitPoseSet.Value, FinalPreProcessCalibration, KMeansClusterCount, KMeansMaxIterations, true);

		FPoseLookupTable& PoseLookupTable = PoseLookupSets.FindOrAdd(TraitPoseSet.Key);

		PoseLookupTable.Process(TraitPoseSet.Value, KMeansClusteringSet, FinalPreProcessCalibration,
			DesiredLookupTableSize);

		//Set the candidate set Id for each pose that is able to be looked up.
		for (int32 i = 0; i < PoseLookupTable.CandidateSets.Num(); ++i)
		{
			FPoseCandidateSet& CandidateSet = PoseLookupTable.CandidateSets[i];
			CandidateSet.SetId = i;

			for (FPoseMotionData& Pose : CandidateSet.PoseCandidates)
			{
				InMotionDataAsset->Poses[Pose.PoseId].CandidateSetId = i;
			}
		}
	}

	//Convert all poses into pose id's for smaller file size
	for (auto& PoseLookupPair : PoseLookupSets)
	{
		FPoseLookupTable& LookupTable = PoseLookupPair.Value;

		for(FPoseCandidateSet& CandidateSet : LookupTable.CandidateSets)
		{
			CandidateSet.SerializeCandidatePoseIds();
		}
	}
}

TArray<FPoseMotionData>* UMMOptimisation_MultiClustering::GetFilteredPoseList(const FPoseMotionData& CurrentPose, 
	const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration)
{
	int32 CandidateSetId = CurrentPose.CandidateSetId;

	//If there is a trait mismatch we need to find the CandidateSetId by doing a pose search on the average pose of any given lookup table.
	if (RequiredTraits != CurrentPose.Traits || CandidateSetId == -1)
	{
		int32 LowestCostCandidateSetId = -1;
		float LowestCost = 100000000.0f;
		for(FPoseCandidateSet& CandidateSet : PoseLookupSets[RequiredTraits].CandidateSets)
		{
			//Body Velocity Cost (Is this needed here?)
			float Cost = FVector::DistSquared(CurrentPose.LocalVelocity, CandidateSet.AveragePose.LocalVelocity) * FinalCalibration.Weight_Momentum;

			//Body Rotational Velocity Cost (Is this needed here?)
			Cost += FMath::Abs(CurrentPose.RotationalVelocity - CandidateSet.AveragePose.RotationalVelocity)
				* FinalCalibration.Weight_AngularMomentum;

			if(Cost > LowestCost) continue; //Early Out

			//Pose Joint Cost
			Cost += FMotionMatchingUtils::ComputePoseCost(CurrentPose.JointData,
				CandidateSet.AveragePose.JointData, FinalCalibration);

			if (Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestCostCandidateSetId = CandidateSet.SetId;
			}
		}

		CandidateSetId = LowestCostCandidateSetId;
	}

	if (PoseLookupSets.Contains(RequiredTraits) && CandidateSetId > -1)
	{
		FPoseLookupTable& LookupTable = PoseLookupSets[RequiredTraits];
		if(CandidateSetId >= LookupTable.CandidateSets.Num())
		{   
			UE_LOG(LogTemp, Warning, TEXT("Could not find pose candidate set for the current pose. Reverting to linear search."))
			return nullptr;
		}
		
		return &LookupTable.CandidateSets[CandidateSetId].PoseCandidates;
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not find pose candidate set for the current pose. Reverting to linear search."))
	return nullptr;
}

void UMMOptimisation_MultiClustering::InitializeRuntime()
{
	if (bIsRuntimeInitialized || !ParentMotionDataAsset)
	{
		return;
	}	

	Super::InitializeRuntime();

	for (auto& PoseLookupPair : PoseLookupSets)
	{
		FPoseLookupTable& LookupTable = PoseLookupPair.Value;
		LookupTable.InitializeRuntime(ParentMotionDataAsset);
	}
}

void UMMOptimisation_MultiClustering::DrawDebug(FPrimitiveDrawInterface* DrawInterface, const UWorld* World, const UMotionDataAsset* MotionData) const
{
	if (!DrawInterface
		|| !World
		|| !MotionData)
	{
		return;
	}

	if (!MotionData->bIsProcessed
		|| !MotionData->bOptimize
		|| MotionData->OptimisationModule != this)
	{
		return;
	}

	//Draw trajectory clusters
#if WITH_EDITORONLY_DATA
	for (int32 i = 0; i < KMeansClusteringSet.Clusters.Num(); ++i)
	{
		const FKMCluster& Cluster = KMeansClusteringSet.Clusters[i];

		

		for (const FPoseMotionData& Pose : Cluster.Samples)
		{
			if (Pose.Trajectory.Num() == 0)
			{
				continue;
			}

			FVector LastPointPos = Pose.Trajectory[0].Position;
			for (int32 k = 0; k < Pose.Trajectory.Num(); ++k)
			{
				FVector PointPos = Pose.Trajectory[k].Position;
				DrawInterface->DrawLine(LastPointPos, PointPos, Cluster.DebugDrawColor, ESceneDepthPriorityGroup::SDPG_Foreground, 0.0f);
				LastPointPos = PointPos;
			}
		}

		/*FVector LastPointPos = Cluster.Center[0].Position;
		for (const FTrajectoryPoint& Center : Cluster.Center)
		{
			DrawInterface->DrawLine(LastPointPos, Center.Position, Cluster.DebugDrawColor, ESceneDepthPriorityGroup::SDPG_Foreground, 2.0f);
			LastPointPos = Center.Position;
		}*/
	}
#endif

	//Todo: Draw pose lookup table data
}
