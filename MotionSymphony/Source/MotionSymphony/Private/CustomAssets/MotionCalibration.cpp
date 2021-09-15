// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MotionCalibration.h"

#define LOCTEXT_NAMESPACE "MotionCalibration"

FJointWeightSet::FJointWeightSet()
	: Weight_Pos(1.0f),
	  Weight_Vel(1.0f)
{}

FJointWeightSet::FJointWeightSet(float InWeightPos, float InWeightVel)
	: Weight_Pos(InWeightPos),
	  Weight_Vel(InWeightVel)
{}

FJointWeightSet FJointWeightSet::operator*(const FJointWeightSet& rhs)
{
	return FJointWeightSet(	Weight_Pos * rhs.Weight_Pos,
							Weight_Vel * rhs.Weight_Vel);
}

FTrajectoryWeightSet::FTrajectoryWeightSet()
	: Weight_Pos(5.0f),
	  Weight_Facing(3.0f)
{}


FTrajectoryWeightSet::FTrajectoryWeightSet(float InWeightPos, float InWeightFacing)
	: Weight_Pos(InWeightPos),
	  Weight_Facing(InWeightFacing)
{}

FTrajectoryWeightSet FTrajectoryWeightSet::operator*(const FTrajectoryWeightSet& rhs)
{
	return FTrajectoryWeightSet(Weight_Pos * rhs.Weight_Pos,
								Weight_Facing * rhs.Weight_Facing);
}

UMotionCalibration::UMotionCalibration(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer),
	MotionMatchConfig(nullptr),
	bOverrideDefaults(false),
	QualityVsResponsivenessRatio(0.5f),
	Weight_Momentum(1.0f),
	Weight_AngularMomentum(1.0f),
	JointPosition_DefaultWeight(1.0),
	JointVelocity_DefaultWeight(1.0f),
	TrajectoryPosition_DefaultWeight(5.0f),
	TrajectoryFacing_DefaultWeight(3.0f),
	bIsInitialized(false)
{}

void UMotionCalibration::Initialize()
{
	if (!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Trying to initialize MotionCalibration but the MotionMatchConfig is null. Please ensure your Motion Calibration has a configuration set."));
		return;
	}

	if(bIsInitialized)
		return;

	ValidateData();

	OnGenerateJointWeightings();
	OnGenerateTrajectoryWeightings();
	OnGeneratePoseWeightings();

	//Pose Ratio Adjustment
	const float PoseAdjustment = (1.0f - QualityVsResponsivenessRatio) * 2.0f;
	for (FJointWeightSet& JointWeightSet : PoseJointWeights)
	{
		JointWeightSet.Weight_Pos *= PoseAdjustment;
		JointWeightSet.Weight_Vel *= PoseAdjustment;
	}

	//Trajectory Ratio Adjustment
	const float TrajAdjustment = QualityVsResponsivenessRatio * 2.0f;
	for (FTrajectoryWeightSet& TrajWeightSet : TrajectoryWeights)
	{
		TrajWeightSet.Weight_Pos *= TrajAdjustment;
		TrajWeightSet.Weight_Facing *= TrajAdjustment;
	}
}

void UMotionCalibration::ValidateData()
{
	if (!MotionMatchConfig)
	{
		//UE_LOG(LogTemp, Error, TEXT("Motion Calibration validation failed. Motion matching config not set"));
		UE_LOG(LogTemp, Warning, TEXT("Motion Calibration validation failed. Motion matching config not set"));
		return;
	}

	int32 PreCount = PoseJointWeights.Num();
	if (MotionMatchConfig->PoseBones.Num() != PoseJointWeights.Num())
	{
		PoseJointWeights.SetNum(MotionMatchConfig->PoseBones.Num());

		for (int32 i = PreCount; i < MotionMatchConfig->PoseBones.Num(); ++i)
		{
			PoseJointWeights[i] = FJointWeightSet(JointPosition_DefaultWeight, JointVelocity_DefaultWeight);
		}

		Modify(true);
	}

	PreCount = TrajectoryWeights.Num();
	if (MotionMatchConfig->TrajectoryTimes.Num() != TrajectoryWeights.Num())
	{
		TrajectoryWeights.SetNum(MotionMatchConfig->TrajectoryTimes.Num());

		for (int32 i = PreCount; i < MotionMatchConfig->TrajectoryTimes.Num(); ++i)
		{
			TrajectoryWeights[i] = FTrajectoryWeightSet(TrajectoryPosition_DefaultWeight, TrajectoryFacing_DefaultWeight);
		}


		Modify(true);
	}
}

bool UMotionCalibration::IsSetupValid(UMotionMatchConfig* InMotionMatchConfig)
{
	if (!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Calibration validity check failed. The MotionMatchConfig property has not been set."));
		return false;
	}
	else if (MotionMatchConfig != InMotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Calibration validity check failed. The MotionMatchConfig property does not match the config set on the MotionData Asset."));
		return false;
	}

	return true;
}

void UMotionCalibration::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	//ValidateData();
}

void UMotionCalibration::OnGenerateJointWeightings_Implementation()
{
	if(bOverrideDefaults)
		return;

	for (FJointWeightSet& JointSet : PoseJointWeights)
	{
		JointSet.Weight_Pos = JointPosition_DefaultWeight;
		JointSet.Weight_Vel = JointVelocity_DefaultWeight;
	}
}

void UMotionCalibration::OnGenerateTrajectoryWeightings_Implementation()
{
	if (bOverrideDefaults)
		return;

	for(int32 i = 0; i < TrajectoryWeights.Num(); ++i)
	{
		FTrajectoryWeightSet& TrajSet = TrajectoryWeights[i];

		TrajSet.Weight_Pos = TrajectoryPosition_DefaultWeight/* * MotionMatchConfig->TrajectoryTimes[i];*/;
		TrajSet.Weight_Facing = TrajectoryFacing_DefaultWeight;
	}
}

void UMotionCalibration::OnGeneratePoseWeightings_Implementation()
{}

#if WITH_EDITORONLY_DATA
void UMotionCalibration::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	ValidateData();
}
#endif

void UMotionCalibration::SetBoneWeightSet(FName BoneName, float Weight_Pos, float Weight_Vel)
{
	//First find the bone
	for(int i = 0; i < MotionMatchConfig->PoseBones.Num(); ++i)
	{
		FBoneReference& BoneRef = MotionMatchConfig->PoseBones[i];

		if (BoneRef.BoneName == BoneName)
		{
			PoseJointWeights[i] = FJointWeightSet(Weight_Pos, Weight_Vel);
			break;
		}
	}
}

void UMotionCalibration::SetTrajectoryWeightSet(int32 Index, float Weight_Pos, float Weight_Facing)
{
	if (Index < 0 || Index >= TrajectoryWeights.Num())
	{
		return;
	}

	TrajectoryWeights[Index] = FTrajectoryWeightSet(Weight_Pos, Weight_Facing);
}

#undef LOCTEXT_NAMESPACE