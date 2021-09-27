// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "KMeansClustering.h"
#include "Data/CalibrationData.h"
#include "MotionMatchingUtil\MotionMatchingUtils.h"

FKMCluster::FKMCluster()
	: Variance(-1.0f)
{

	DebugDrawColor = FColor::MakeRandomColor();
}

FKMCluster::FKMCluster(FPoseMotionData & BasePose, int32 EstimatedSamples)
	: Variance(-1.0f)
{
	Samples.Empty(EstimatedSamples + 1);

	DebugDrawColor = FColor::MakeRandomColor();
	
	for (int32 i = 0; i < BasePose.Trajectory.Num(); ++i)
	{
		Center.Add(BasePose.Trajectory[i]);
	}
}

float FKMCluster::ComputePoseCost(FPoseMotionData & Pose, FCalibrationData& Calibration)
{
	return FMotionMatchingUtils::ComputeTrajectoryCost(Pose.Trajectory, Center, Calibration) * Pose.Favour;
}

float FKMCluster::ReCalculateCenter()
{
	//Don't re-calculate the center if there are no samples. Keep it as is.
	if (Samples.Num() < 1)
	{
		return 0.0f;
	}

	//If there is only 1 sample, make it the center and avoid calculating averages.
	if (Samples.Num() == 1)
	{
		FPoseMotionData& Sample = Samples[0];

		for (int32 i = 0; i < Center.Num(); ++i)
		{
			Center[i] = Sample.Trajectory[i];
		}

		return 0.0f;
	}


	TArray<FTrajectoryPoint> CumulativeTrajectory;
	CumulativeTrajectory.Empty(Center.Max());

	for (int32 i = 0; i < Center.Num(); ++i)
	{
		CumulativeTrajectory.Emplace(FTrajectoryPoint(FVector::ZeroVector, 0.0f));
	}

	//Add up all the trajectory points
	for (FPoseMotionData& Pose : Samples)
	{
		//Add up all the trajectories from all the pose samples
		for (int32 i = 0; i < Pose.Trajectory.Num(); ++i)
		{
			FTrajectoryPoint& CumPoint = CumulativeTrajectory[i];
			FTrajectoryPoint& SamplePoint = Pose.Trajectory[i];

			CumPoint.Position += SamplePoint.Position;
			CumPoint.RotationZ += SamplePoint.RotationZ;
		}	
	}

	//Average the cumulative trajectory to find the new center of the cluster
	for (int32 i = 0; i < CumulativeTrajectory.Num(); ++i)
	{
		FTrajectoryPoint& CumPoint = CumulativeTrajectory[i];

		CumPoint.Position /= Samples.Num();
		CumPoint.RotationZ /= Samples.Num();
	}

	const float CenterDelta = FMotionMatchingUtils::ComputeTrajectoryCost(Center, CumulativeTrajectory, 1.0f, 1.0f);

	for (int32 i = 0; i < Center.Num(); ++i)
	{
		Center[i] = CumulativeTrajectory[i];
	}

	//Calculate the difference between the old and new center
	return CenterDelta;
}


void FKMCluster::AddPose(FPoseMotionData& Pose)
{
	Samples.Add(Pose);
}

float FKMCluster::CalculateVariance()
{
	Variance = -10000000.0f;
	for (int32 i = 0; i < Samples.Num(); ++i)
	{
		for (int32 k = 0; k < Samples.Num(); ++k)
		{
			if(k == i)
			{
				continue; //Don't bother calculating against self
			}

			const float AtomVariance = FMotionMatchingUtils::ComputeTrajectoryCost(Samples[i].Trajectory, Samples[k].Trajectory, 1.0f, 0.0f);

			if (AtomVariance > Variance)
			{
				Variance = AtomVariance;
			}
			
		}
	}

	return Variance;
}

void FKMCluster::Reset()
{
	Samples.Empty(Samples.Max());
}

FKMeansClusteringSet::FKMeansClusteringSet()
	: K(200),
	  Variance(0.0f),
	  Calibration(nullptr)
{
}

void FKMeansClusteringSet::BeginClustering(TArray<FPoseMotionData>& Poses, FCalibrationData& InCalibration,
	const int32 InK, const int32 MaxIterations, const bool bFast /* = false*/)
{
	if(Poses.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("KMeansClusteringSet: Failed to cluster with zero poses"));
		return;
	}

	if (K >= Poses.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("KMeansClusteringSet: K value is too high as it is greater or equal to the number of poses"));
		return;
	}

	Calibration = &InCalibration;

	K = InK > 0 ? InK : 1;
	Clusters.Empty(K + 1);

	if (bFast)
	{
		InitializeClustersFast(Poses);

		//Continuously process the clusters until MaxIterations is reached or until the clusters no longer change
		for (int32 i = 0; i < MaxIterations; ++i)
		{
			if (!ProcessClusters(Poses))
			{
				//If no cluster has changed this iteration, stop iterating
				break;
			}
		}
	}
	else
	{
		InitializeClusters(Poses);
		ProcessClusters(Poses); //The clusters should be evently spaced so it should not need to be run multiple times
	}

	//Calculate the quality of the clustering and store it. This is so we can run the clustering a number of times 
	//and see which clustering attempt was the best.
}

float FKMeansClusteringSet::CalculateVariance()
{
	int32 LowestVariance = 10000000.0f;
	int32 HighestVariance = -10000000.0f;
	for (FKMCluster& Cluster : Clusters)
	{
		const float ClusterVariance = Cluster.CalculateVariance();

		if (ClusterVariance < LowestVariance)
			LowestVariance = ClusterVariance;
		

		if (ClusterVariance > HighestVariance)
			HighestVariance = ClusterVariance;
		
	}

	Variance = HighestVariance - LowestVariance;

	return Variance;
}

void FKMeansClusteringSet::InitializeClusters(TArray<FPoseMotionData>& Poses)
{
	const int32 EstimatedSamples = Poses.Num() / K * 2;

	TArray<FPoseMotionData*> PosesCopy;
	PosesCopy.Empty(Poses.Num() + 1);
	for (FPoseMotionData& Pose : Poses)
	{
		PosesCopy.Add(&Pose);
	}

	const int32 RandomStartingCluster = FMath::RandRange(0, Poses.Num() - 1);

	Clusters.Emplace(FKMCluster(Poses[RandomStartingCluster], EstimatedSamples));
	PosesCopy.RemoveAt(RandomStartingCluster);

	for (int32 i = 1; i < K; ++i)
	{
		float HighestPoseCost = -20000000.0f;
		int32 HighestPoseId = -1;
		for (int32 k = 0; k < PosesCopy.Num(); ++k)
		{
			float LowestCost = 20000000.0f;
			for (int32 j = 0; j < Clusters.Num(); ++j)
			{
				const float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(Clusters[j].Center,
				                                                               PosesCopy[k]->Trajectory, *Calibration);

				if (Cost < LowestCost)
				{
					LowestCost = Cost;
				}
			}

			if (LowestCost > HighestPoseCost)
			{
				HighestPoseCost = LowestCost;
				HighestPoseId = k;
			}
		}

		//This can occur if all trajectories are zero. i.e. there is no root motion
		if (HighestPoseId < 0 || HighestPoseId >= PosesCopy.Num())
		{
			UE_LOG(LogTemp, Warning, TEXT("Trajectory clustering failed. Optimisation will not process properly Is your root motion setup properly?"));
			HighestPoseId = 0;
		}

		//Ok HighestPoseId is a new cluster center
		Clusters.Emplace(FKMCluster(*PosesCopy[HighestPoseId], EstimatedSamples));
		PosesCopy.RemoveAt(HighestPoseId);
	}
}

void FKMeansClusteringSet::InitializeClustersFast(TArray<FPoseMotionData>& Poses)
{
	const int32 EstimatedSamples = Poses.Num() / K * 2;

	TArray<int32> RandomIndexesUsed;
	RandomIndexesUsed.Empty(K + 1);

	//Pick K random samples from the pose list and for each one initialize a cluster
	for (int32 i = 0; i < K; ++i)
	{
		//Find a random index to use as the cluster but make sure that it is not already used
		int32 RandomIndex = 0;
		while (true)
		{
			RandomIndex = FMath::RandRange(0, Poses.Num() - 1);

			bool bAlreadyUsed = false;
			if (Poses[RandomIndex].bDoNotUse)
			{
				bAlreadyUsed = true;
				continue;
			}


			for (int k = 0; k < RandomIndexesUsed.Num(); ++k)
			{
				if (RandomIndex == RandomIndexesUsed[k])
				{
					bAlreadyUsed = true;
					break;
				}
			}

			if (!bAlreadyUsed)
			{
				break;
			}
		}

		//Create Cluster for the random index pose
		Clusters.Emplace(FKMCluster(Poses[RandomIndex], EstimatedSamples));
	}
}

bool FKMeansClusteringSet::ProcessClusters(TArray<FPoseMotionData>& Poses)
{
	for(FKMCluster& Cluster : Clusters)
	{
		Cluster.Reset();
	}

	//cycle through every pose and find which cluster it fits into based on distance (trajectory comparison)
	for (FPoseMotionData& Pose : Poses)
	{
		//We won't bother clustering bDoNotUse poses
		if(Pose.bDoNotUse)
		{
			continue;
		}

		//Cost function to find the best cluster for this pose to fit in
		float LowestClusterCost = 1000000.0f;
		int32 LowestClusterId = -1;
		for (int32 i = 0; i < Clusters.Num(); ++i)
		{
			const float Cost = Clusters[i].ComputePoseCost(Pose, *Calibration);

			if (Cost < LowestClusterCost)
			{
				LowestClusterCost = Cost;
				LowestClusterId = i;
			}
		}

		//Add the pose to the closest cluster
		if(LowestClusterId > -1 && LowestClusterId < Clusters.Num())
		{
			Clusters[LowestClusterId].AddPose(Pose);
		}
	}

	bool bClustersChanged = false;
	for (FKMCluster& Cluster : Clusters)
	{
		const float ClusterDeltaTolerance = 1.0f;
		if (Cluster.ReCalculateCenter() > ClusterDeltaTolerance)
		{
			bClustersChanged = true;
		}
	}

	return bClustersChanged;
}

void FKMeansClusteringSet::Clear()
{
	Clusters.Empty(Clusters.Num());
	Calibration = nullptr;
}
