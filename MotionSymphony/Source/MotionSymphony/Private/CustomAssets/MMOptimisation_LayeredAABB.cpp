// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MMOptimisation_LayeredAABB.h"
#include "CustomAssets/MotionDataAsset.h"

UMMOptimisation_LayeredAABB::UMMOptimisation_LayeredAABB(const FObjectInitializer& ObjectInitializer)
	: UMMOptimisationModule(ObjectInitializer)
{

}

void UMMOptimisation_LayeredAABB::BuildOptimisationStructures(UMotionDataAsset* InMotionDataAsset)
{
	Super::BuildOptimisationStructures(InMotionDataAsset);

	SearchStructure.Empty();

	//First Create trait bins to separate poses by trait (Cull all DoNotUse poses)
	TMap<FMotionTraitField, TArray<FPoseMotionData>> PoseBins;

	for (FPoseMotionData& Pose : InMotionDataAsset->Poses)
	{
		TArray<FPoseMotionData>& PoseBin = PoseBins.FindOrAdd(Pose.Traits);
		PoseBin.Add(Pose);
	}

	//For each trait bin we make a new AABB search structure
	for (auto& TraitPoseSet : PoseBins)
	{
		FLayeredAABBStructure& LayeredAABBStructure = SearchStructure.FindOrAdd(TraitPoseSet.Key);

		for (FPoseMotionData& Pose : TraitPoseSet.Value)
		//for(int32 i = 0; i < TraitPoseSet.Value.Num(); ++i)
		{
			//FPoseMotionData& Pose = TraitPoseSet.Value[i];

			if (Pose.bDoNotUse)
			{
				continue;
			}

			LayeredAABBStructure.AddPose(Pose);
		}

		LayeredAABBStructure.CalculateAABBs();
	}
}

TArray<FPoseMotionData>* UMMOptimisation_LayeredAABB::GetFilteredPoseList(const FPoseMotionData& CurrentPose,
	const FMotionTraitField RequiredTraits, const FCalibrationData& FinalCalibration)
{
	return nullptr;
}

void FLayeredAABBStructure::AddPose(FPoseMotionData& Pose)
{
	if (ChildAABBs.Num() == 0)
	{
		ChildAABBs.Emplace(FPoseAABBCollection());
	}

	FPoseAABBCollection& ChildAABB = ChildAABBs.Last(); 

	if (!ChildAABB.IsFull())
	{
		//Add the pose to the latest AABB because it is not full
		ChildAABB.AddPose(Pose);
	}
	else
	{
		//All AABBs are full, create a new one
		ChildAABBs.Emplace(FPoseAABBCollection());
		ChildAABBs.Last().AddPose(Pose);
	}
}

void FLayeredAABBStructure::CalculateAABBs()
{
	for (FPoseAABBCollection& AABBCollection : ChildAABBs)
	{
		AABBCollection.CalculateAABB();
	}
}

void FPoseAABBCollection::AddPose(FPoseMotionData& Pose)
{
	if (ChildAABBs.Num() == 0)
	{
		ChildAABBs.Emplace(FPoseAABB());
	}

	FPoseAABB& ChildAABB = ChildAABBs.Last();

	if (!ChildAABB.IsFull())
	{
		ChildAABB.AddPose(Pose);
	}
	else
	{
		ChildAABBs.Emplace(FPoseAABB());
		ChildAABBs.Last().AddPose(Pose);
	}
}

void FPoseAABBCollection::CalculateAABB()
{
	for (FPoseAABB& PoseAABB : ChildAABBs)
	{
		PoseAABB.CalculateAABB();
	}
	//Based on the child AABBs, now calculate this AABB
}

bool FPoseAABBCollection::IsFull()
{
	//Only 4 child AABBs in a parent AABB
	if (ChildAABBs.Num() < 4)
	{
		return false;
	}

	//If the latest AABB is full then this AABBCollection is full
	return ChildAABBs.Last().IsFull();
}

void FPoseAABB::AddPose(FPoseMotionData& Pose)
{
	Poses.Add(Pose);
}

void FPoseAABB::CalculateAABB()
{
	//Find the center of the AABB by averaging all poses
	Center = Poses[0];
	for (int32 i = 1; i < Poses.Num(); ++i)
	{
		Center += Poses[i];
	}
	Center /= Poses.Num();

	//Find the extents of the AABB (i.e. the 
	Extents = Center;
	Extents.Clear();

	for (FPoseMotionData& Pose : Poses)
	{
		//Find extents of velocity
		FVector ExtentsMomentum = Extents.LocalVelocity;

		FVector MomentumDif = Pose.LocalVelocity - Center.LocalVelocity;
		ExtentsMomentum.X = FMath::Abs(MomentumDif.X) > ExtentsMomentum.X ? FMath::Abs(MomentumDif.X) : ExtentsMomentum.X;
		ExtentsMomentum.Y = FMath::Abs(MomentumDif.Y) > ExtentsMomentum.Y ? FMath::Abs(MomentumDif.Y) : ExtentsMomentum.Y;
		ExtentsMomentum.Z = FMath::Abs(MomentumDif.Z) > ExtentsMomentum.Z ? FMath::Abs(MomentumDif.Z) : ExtentsMomentum.Z;

		//Find extents of rotational velocity
		float RotVelDif = Pose.RotationalVelocity - Center.RotationalVelocity;
		Extents.RotationalVelocity = FMath::Abs(RotVelDif) > Extents.RotationalVelocity ? FMath::Abs(RotVelDif) : Extents.RotationalVelocity;

		//Find extents of trajectory
		for (int32 i = 0; i < Pose.Trajectory.Num(); ++i)
		{
			FTrajectoryPoint& Point = Pose.Trajectory[i];
			FTrajectoryPoint& ExtentsPoint = Extents.Trajectory[i];
			FTrajectoryPoint& CenterPoint = Center.Trajectory[i];

			FVector PosDif = Point.Position - CenterPoint.Position;
			ExtentsPoint.Position.X = FMath::Abs(PosDif.X) > ExtentsPoint.Position.X ? FMath::Abs(PosDif.X) : ExtentsPoint.Position.X;
			ExtentsPoint.Position.Y = FMath::Abs(PosDif.Y) > ExtentsPoint.Position.Y ? FMath::Abs(PosDif.Y) : ExtentsPoint.Position.Y;
			ExtentsPoint.Position.Z = FMath::Abs(PosDif.Z) > ExtentsPoint.Position.Z ? FMath::Abs(PosDif.Z) : ExtentsPoint.Position.Z;
			
			float RotDif = FMath::Abs(Point.RotationZ - CenterPoint.RotationZ);
			ExtentsPoint.RotationZ = RotDif > ExtentsPoint.RotationZ ?  RotDif : ExtentsPoint.RotationZ;
		}

		//Find extents of pose
		for (int32 i = 0; i < Pose.JointData.Num(); ++i)
		{
			FJointData& Joint = Pose.JointData[i];
			FJointData& ExtentsJoint = Extents.JointData[i];
			FJointData& CenterJoint = Center.JointData[i];

			FVector PosDif = Joint.Position - CenterJoint.Position;
			FVector VelDif = Joint.Position - CenterJoint.Velocity;

			ExtentsJoint.Position.X = FMath::Abs(PosDif.X) > ExtentsJoint.Position.X ? FMath::Abs(PosDif.X) : ExtentsJoint.Position.X;
			ExtentsJoint.Position.Y = FMath::Abs(PosDif.Y) > ExtentsJoint.Position.Y ? FMath::Abs(PosDif.Y) : ExtentsJoint.Position.Y;
			ExtentsJoint.Position.Z = FMath::Abs(PosDif.Z) > ExtentsJoint.Position.Z ? FMath::Abs(PosDif.Z) : ExtentsJoint.Position.Z;

			ExtentsJoint.Velocity.X = FMath::Abs(VelDif.X) > ExtentsJoint.Velocity.X ? FMath::Abs(VelDif.X) : ExtentsJoint.Velocity.X;
			ExtentsJoint.Velocity.Y = FMath::Abs(VelDif.Y) > ExtentsJoint.Velocity.Y ? FMath::Abs(VelDif.Y) : ExtentsJoint.Velocity.Y;
			ExtentsJoint.Velocity.Z = FMath::Abs(VelDif.Z) > ExtentsJoint.Velocity.Z ? FMath::Abs(VelDif.Z) : ExtentsJoint.Velocity.Z;
		}
	}
}

bool FPoseAABB::IsFull()
{
	//If the PoseAABB has 16 poses then it is already full;
	return Poses.Num() < 16;
}
