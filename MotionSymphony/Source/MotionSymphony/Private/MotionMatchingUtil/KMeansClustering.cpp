// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "KMeansClustering.h"
#include "MotionMatchingUtil\MotionMatchingUtils.h"

FKMCluster::FKMCluster()
	: Variance(-1.0f)
{
}

FKMCluster::FKMCluster(FPoseMotionData & BasePose, int32 EstimatedSamples)
	: Variance(-1.0f)
{
	Samples.Empty(EstimatedSamples + 1);
	
	for (int32 i = 0; i < BasePose.Trajectory.Num(); ++i)
	{
		Center.Add(BasePose.Trajectory[i]);
	}
}

float FKMCluster::ComputePoseCost(FPoseMotionData & Pose, const float PosWeight, const float RotWeight)
{
	return FMotionMatchingUtils::ComputeTrajectoryCost(Pose.Trajectory, Center, PosWeight, RotWeight) * Pose.Favour;
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
		FPoseMotionData* Sample = Samples[0];

		for (int32 i = 0; i < Center.Num(); ++i)
		{
			Center[i] = Sample->Trajectory[i];
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
	for (FPoseMotionData* Pose : Samples)
	{
		if (Pose)
		{
			//Add up all the trajectories from all the pose samples
			for (int32 i = 0; i < Pose->Trajectory.Num(); ++i)
			{
				FTrajectoryPoint& CumPoint = CumulativeTrajectory[i];
				FTrajectoryPoint& SamplePoint = Pose->Trajectory[i];

				CumPoint.Position += SamplePoint.Position;
				CumPoint.RotationZ += SamplePoint.RotationZ; //Todo:: This probably should be converted to a quaternion
			}
		}
	}


	//Average the cumulative trajectory to find the new center of the cluster
	for (int32 i = 0; i < CumulativeTrajectory.Num(); ++i)
	{
		FTrajectoryPoint& CumPoint = CumulativeTrajectory[i];

		CumPoint.Position /= Samples.Num();
		CumPoint.RotationZ /= Samples.Num();
	}

	float CenterDelta = FMotionMatchingUtils::ComputeTrajectoryCost(Center, CumulativeTrajectory, 1.0f, 1.0f);

	for (int32 i = 0; i < Center.Num(); ++i)
	{
		Center[i] = CumulativeTrajectory[i];
	}

	//Calculate the difference between the old and new center
	return CenterDelta;
}


void FKMCluster::AddPose(FPoseMotionData& Pose)
{
	Samples.Add(&Pose);
}

float FKMCluster::CalculateVariance()
{
	DebugDrawColor = FColor::MakeRandomColor();

	Variance = -10000000.0f;
	for (int32 i = 0; i < Samples.Num(); ++i)
	{
		for (int32 k = 0; k < Samples.Num(); ++k)
		{
			if(k == i)
				continue; //Don't bother calculating against self

			float AtomVariance = FMotionMatchingUtils::ComputeTrajectoryCost(Samples[i]->Trajectory, Samples[k]->Trajectory, 1.0f, 0.0f);


			if (AtomVariance > Variance)
				Variance = AtomVariance;
			
		}
	}

	return Variance;
}

void FKMCluster::Reset()
{
	int32 Capacity = Samples.Max();
	Samples.Empty(Capacity);
}

FKMeansClusteringSet::FKMeansClusteringSet()
	: K(200)
{
}

void FKMeansClusteringSet::BeginClustering(TArray<FPoseMotionData>& Poses, const FCalibrationData& InCalibration, int32 InK, int32 MaxIterations, bool bFast /* = false*/)
{
	if(Poses.Num() == 0)
	{
		//Failed to cluster with zero poses
		return;
	}

	if (K == Poses.Num())
	{
		//Cannot have K as the number of poses
		return;
	}

	Calibration = InCalibration;

	K = InK > 0 ? InK : 1;
	Clusters.Empty(K + 1);

	if (bFast)
	{
		InitializeClustersFast(Poses);
	}
	else
	{
		InitializeClusters(Poses);
	}
	
	//Continuously process the clusters until MaxIterations is reached or until the clusters no longer change
	for (int32 i = 0; i < MaxIterations; ++i)
	{
		if (!ProcessClusters(Poses))
		{
			//If no cluster has changed this iteration, stop iterating
			break;
		}
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
		float ClusterVariance = Cluster.CalculateVariance();

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
	int32 EstimatedSamples = Poses.Num() / K * 2;

	TArray<FPoseMotionData*> PosesCopy;
	PosesCopy.Empty(Poses.Num() + 1);
	for (FPoseMotionData& Pose : Poses)
	{
		PosesCopy.Add(&Pose);
	}

	int32 RandomStartingCluster = FMath::RandRange(0, Poses.Num() - 1);

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
				float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(Clusters[j].Center, PosesCopy[K]->Trajectory, 
					Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

				if(Cost < LowestCost)
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

		//Ok HighestPoseId is a new cluster center
		Clusters.Emplace(FKMCluster(*PosesCopy[HighestPoseId], EstimatedSamples));
		PosesCopy.RemoveAt(HighestPoseId);
	}
}

void FKMeansClusteringSet::InitializeClustersFast(TArray<FPoseMotionData>& Poses)
{
	int32 EstimatedSamples = Poses.Num() / K * 2;

	TArray<int32> RandomIndexesUsed;
	RandomIndexesUsed.Empty(K + 1);

	//Pick K random samples from the pose list and for each one initialize a cluster
	for (int32 i = 0; i < K; ++i)
	{
		//Find a random index to use as the cluster but make sure that it is not already used
		int32 RandomIndex;
		while (true)
		{
			RandomIndex = FMath::RandRange(0, Poses.Num() - 1);

			bool bAlreadyUsed = false;
			if (Poses[RandomIndex].DoNotUse)
			{
				bAlreadyUsed = true;
				continue;
			}


			for (int k = 0; k < RandomIndexesUsed.Num(); ++i)
			{
				if (RandomIndex == RandomIndexesUsed[i])
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

	int32 NumClusters = Clusters.Num();

	//cycle through every pose and find which cluster it fits into based on distance (trajectory comparison)
	for (FPoseMotionData& Pose : Poses)
	{
		//We won't bother clustering DoNotUse poses
		if(Pose.DoNotUse)
			continue;

		//Cost function to find the best cluster for this pose to fit in
		float LowestClusterCost = 1000000.0f;
		int32 LowestClusterId = -1;
		for (int32 i = 0; i < NumClusters; ++i)
		{
			float Cost = Clusters[i].ComputePoseCost(Pose, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

			if (Cost < LowestClusterCost)
			{
				LowestClusterCost = Cost;
				LowestClusterId = i;
			}
		}

		//Add the pose to the closest cluster
		Clusters[LowestClusterId].AddPose(Pose);
	}

	bool bClustersChanged = false;
	float ClusterDeltaTolerance = 1.0f;
	for (FKMCluster& Cluster : Clusters)
	{
		if (Cluster.ReCalculateCenter() > ClusterDeltaTolerance)
		{
			bClustersChanged = true;
		}
	}

	return bClustersChanged;
}