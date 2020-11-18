// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatchBase.h"
#include "MMPreProcessUtils.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"
#include "DrawDebugHelpers.h"
#include "AnimNode_MotionRecorder.h"

#define LOCTEXT_NAMESPACE "MotionSymphonyNodes"

static TAutoConsoleVariable<int32> CVarPoseMatchingDebug(
	TEXT("a.AnimNode.MoSymph.PoseMatch.Debug"),
	0,
	TEXT("Turns Pose Matching Debugging On / Off.\n")
	TEXT("<=0: Off \n")
	TEXT("  1: On - Show chosen pose\n")
	TEXT("  2: On - Show All Poses\n")
	TEXT("  3: On - Show All Poses With Velocity\n"));


FPoseMatchData::FPoseMatchData()
	: PoseId(-1),
	AnimId(0),
	Time(0.0f),
	LocalVelocity(FVector::ZeroVector)
{
}

FPoseMatchData::FPoseMatchData(int32 InPoseId, int32 InAnimId, float InTime, FVector& InLocalVelocity)
	: PoseId(InPoseId),
	AnimId(InAnimId),
	Time(InTime),
	LocalVelocity(InLocalVelocity)
{
}

FMatchBone::FMatchBone()
	: PositionWeight(1.0f),
	VelocityWeight(1.0f)
{
}


FAnimNode_PoseMatchBase::FAnimNode_PoseMatchBase()
	: PoseInterval(0.1f),
	PoseMatchMethod(EPoseMatchMethod::HighQuality),
	PosesEndTime(5.0f),
	BodyVelocityWeight(1.0f),
	bInitialized(false),
	bInitPoseSearch(false),
	CurrentLocalVelocity(FVector::ZeroVector),
	MatchPose(nullptr),
	AnimInstanceProxy(nullptr)
{
}

void FAnimNode_PoseMatchBase::PreProcess()
{
	Poses.Empty();
}

void FAnimNode_PoseMatchBase::PreProcessAnimation(UAnimSequence* Anim, int32 AnimIndex)
{
	if(!Anim || PoseConfiguration.Num() == 0)
		return;

	const float AnimLength = FMath::Min(Anim->SequenceLength, PosesEndTime);
	float CurrentTime = 0.0f;
	
	if(PoseInterval < 0.01f)
		PoseInterval = 0.05f;

	while (CurrentTime <= AnimLength)
	{
		int32 PoseId = Poses.Num();

		FVector RootVelocity;
		FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, Anim, CurrentTime, PoseInterval);

		FPoseMatchData NewPoseData = FPoseMatchData(PoseId, AnimIndex, CurrentTime, RootVelocity);

		//Process Joints for Pose
		for (int32 i = 0; i < PoseConfiguration.Num(); ++i)
		{
			FJointData BoneData;
			FMMPreProcessUtils::ExtractJointData(BoneData, Anim, PoseConfiguration[i].Bone, CurrentTime, PoseInterval);
			NewPoseData.BoneData.Add(BoneData);
		}

		Poses.Add(NewPoseData);
		CurrentTime += PoseInterval;
	}
	
}

void FAnimNode_PoseMatchBase::FindMatchPose(const FAnimationUpdateContext& Context)
{
	if(Poses.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FAnimNode_PoseMatchBase: No poses recorded in node"))
		return;
	}

	FAnimNode_MotionRecorder* MotionRecorderNode = Context.GetAncestor<FAnimNode_MotionRecorder>();

	if (MotionRecorderNode)
	{
		if (!bInitialized)
		{
			for (FMatchBone& MatchBone : PoseConfiguration)
			{
				MotionRecorderNode->RegisterBoneToRecord(MatchBone.Bone);
			}

			bInitialized = true;
		}

		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());

		int32 MinimaCostPoseId = -1;

		switch (PoseMatchMethod)
		{
			case EPoseMatchMethod::LowQuality: { MinimaCostPoseId = GetMinimaCostPoseId_LQ(); } break;
			case EPoseMatchMethod::HighQuality: { MinimaCostPoseId = GetMinimaCostPoseId_HQ(); } break;
		}

		MinimaCostPoseId = FMath::Clamp(MinimaCostPoseId, 0, Poses.Num() - 1);

		MatchPose = &Poses[MinimaCostPoseId];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FAnimNode_PoseMatchBase: Cannot find Motion Snapshot node to pose match against."))
		MatchPose = &Poses[0];
	}

	Sequence = FindActiveAnim();
	InternalTimeAccumulator = StartPosition = MatchPose->Time;
	PlayRateScaleBiasClamp.Reinitialize();
}

UAnimSequenceBase* FAnimNode_PoseMatchBase::FindActiveAnim()
{
	return nullptr;
}

void FAnimNode_PoseMatchBase::ComputeCurrentPose(const FCachedMotionPose& MotionPose)
{
	for (int32 i = 0; i < PoseConfiguration.Num(); ++i)
	{
		int32 BoneIndex = PoseConfiguration[i].Bone.BoneIndex;
		int32 RemappedIndex = MotionPose.MeshToRefSkelMap[BoneIndex];

		if(const FCachedMotionBone* CachedMotionBone = MotionPose.CachedBoneData.Find(RemappedIndex))
		{
			CurrentPose[i] = FJointData(CachedMotionBone->Transform.GetLocation(), CachedMotionBone->Velocity);
		}
	}
}

int32 FAnimNode_PoseMatchBase::GetMinimaCostPoseId_LQ()
{
	int32 MinimaCostPoseId = 0;
	float MinimaCost = 10000000.0f;
	for (FPoseMatchData& Pose : Poses)
	{
		float Cost = 0.0f;

		for (int32 i = 0; i < Pose.BoneData.Num(); ++i)
		{
			const FJointData& CurrentJoint = CurrentPose[i];
			const FJointData& CandidateJoint = Pose.BoneData[i];
			const FMatchBone& MatchBoneInfo = PoseConfiguration[i];

			Cost += FVector::Distance(CurrentJoint.Velocity, CandidateJoint.Velocity) * MatchBoneInfo.VelocityWeight;
			Cost += FVector::Distance(CurrentJoint.Position, CandidateJoint.Position) * MatchBoneInfo.PositionWeight;
		}

		//Todo: Re-Add this once the motion recorder records character velocity
		//Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		if (Cost < MinimaCost)
		{
			MinimaCost = Cost;
			MinimaCostPoseId = Pose.PoseId;
		}
	}

	return MinimaCostPoseId;
}

int32 FAnimNode_PoseMatchBase::GetMinimaCostPoseId_HQ()
{
	int32 MinimaCostPoseId = 0;
	float MinimaCost = 10000000.0f;
	for (FPoseMatchData& Pose : Poses)
	{
		float Cost = 0.0f;

		for (int32 i = 0; i < Pose.BoneData.Num(); ++i)
		{
			const FJointData& CurrentJoint = CurrentPose[i];
			const FJointData& CandidateJoint = Pose.BoneData[i];
			const FMatchBone& MatchBoneInfo = PoseConfiguration[i];

			//Cost of velocity comparison
			Cost += FVector::Distance(CurrentJoint.Velocity, CandidateJoint.Velocity) * MatchBoneInfo.VelocityWeight;

			//Cost of distance between joints
			FVector JointDiff = (CandidateJoint.Position - CurrentJoint.Position);
			Cost += JointDiff.Size() * MatchBoneInfo.PositionWeight;

			//Cost of resultant velocity comparison
			FVector JointResultVel = JointDiff / PoseInterval;
			Cost += FVector::Distance(CurrentJoint.Velocity, JointResultVel) * (MatchBoneInfo.VelocityWeight / 10.0f);
		}

		//Todo: Re-Add this once the motion recorder records character velocity
		//Cost += FVector::Distance(LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		if (Cost < MinimaCost)
		{
			MinimaCost = Cost;
			MinimaCostPoseId = Pose.PoseId;
		}
	}

	return MinimaCostPoseId;
}

int32 FAnimNode_PoseMatchBase::GetMinimaCostPoseId_LQ(float& OutCost, int32 StartPose, int32 EndPose)
{
	if (Poses.Num() == 0)
		return 0;

	StartPose = FMath::Clamp(StartPose, 0, Poses.Num() - 1);
	EndPose = FMath::Clamp(EndPose, 0, Poses.Num() - 1);

	int32 MinimaCostPoseId = 0;
	OutCost = 10000000.0f;
	for (int32 i = StartPose; i < EndPose; ++i)
	{
		FPoseMatchData& Pose = Poses[i];
		
		float Cost = 0.0f;
		for (int32 k = 0; k < Pose.BoneData.Num(); ++k)
		{
			const FJointData& CurrentJoint = CurrentPose[k];
			const FJointData& CandidateJoint = Pose.BoneData[k];
			const FMatchBone& MatchBoneInfo = PoseConfiguration[k];

			Cost += FVector::Distance(CurrentJoint.Velocity, CandidateJoint.Velocity) * MatchBoneInfo.VelocityWeight;
			Cost += FVector::Distance(CurrentJoint.Position, CandidateJoint.Position) * MatchBoneInfo.PositionWeight;
		}

		//Todo: Re-Add this once the motion recorder records character velocity
		//Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		if (Cost < OutCost)
		{
			OutCost = Cost;
			MinimaCostPoseId = Pose.PoseId;
		}
	}

	return MinimaCostPoseId;
}

int32 FAnimNode_PoseMatchBase::GetMinimaCostPoseId_HQ(float& OutCost, int32 StartPose, int32 EndPose)
{
	StartPose = FMath::Clamp(StartPose, 0, Poses.Num());
	EndPose = FMath::Clamp(EndPose, 0, Poses.Num());

	int32 MinimaCostPoseId = 0;
	OutCost = 10000000.0f;
	for (int32 i = StartPose; i < EndPose; ++i)
	{
		FPoseMatchData& Pose = Poses[i];

		float Cost = 0.0f;
		for (int32 k = 0; k < Pose.BoneData.Num(); ++k)
		{
			const FJointData& CurrentJoint = CurrentPose[k];
			const FJointData& CandidateJoint = Pose.BoneData[k];
			const FMatchBone& MatchBoneInfo = PoseConfiguration[k];

			//Cost of velocity comparison
			Cost += FVector::Distance(CurrentJoint.Velocity, CandidateJoint.Velocity) * MatchBoneInfo.VelocityWeight;

			//Cost of distance between joints
			FVector JointDiff = (CandidateJoint.Position - CurrentJoint.Position);
			Cost += JointDiff.Size() * MatchBoneInfo.PositionWeight;

			//Cost of resultant velocity comparison
			FVector JointResultVel = JointDiff / PoseInterval;
			Cost += FVector::Distance(CurrentJoint.Velocity, JointResultVel) * (MatchBoneInfo.VelocityWeight / 10.0f);
		}

		//Todo: Re-Add this once the motion recorder records character velocity
		//Cost += FVector::Distance(LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		if (Cost < OutCost)
		{
			OutCost = Cost;
			MinimaCostPoseId = Pose.PoseId;
		}
	}

	return MinimaCostPoseId;
}

void FAnimNode_PoseMatchBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	bInitPoseSearch = true;
	AnimInstanceProxy = Context.AnimInstanceProxy;
}

void FAnimNode_PoseMatchBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{

}

void FAnimNode_PoseMatchBase::UpdateAssetPlayer(const FAnimationUpdateContext & Context)
{
	//GetEvaluateGraphExposedInputs().Execute(Context);

	if (bInitPoseSearch)
	{
		FindMatchPose(Context); //Override this to setup the animation data

		if (MatchPose && Sequence)
		{
			InternalTimeAccumulator = StartPosition = FMath::Clamp(StartPosition, 0.0f, Sequence->SequenceLength);
			const float AdjustedPlayRate = PlayRateScaleBiasClamp.ApplyTo(FMath::IsNearlyZero(PlayRateBasis) ? 0.0f : (PlayRate / PlayRateBasis), 0.0f);
			const float EffectivePlayrate = Sequence->RateScale * AdjustedPlayRate;
			if ((MatchPose->Time == 0.0f) && (EffectivePlayrate < 0.0f))
			{
				InternalTimeAccumulator = Sequence->SequenceLength;
			}

		}

		bInitPoseSearch = false;
	}

	FAnimNode_SequencePlayer::UpdateAssetPlayer(Context);

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	if (AnimInstanceProxy && MatchPose)
	{
		const USkeletalMeshComponent* SkelMeshComp = AnimInstanceProxy->GetSkelMeshComponent();
		int32 DebugLevel = CVarPoseMatchingDebug.GetValueOnAnyThread();

		if (DebugLevel > 0)
		{
			FTransform ComponentTransform = AnimInstanceProxy->GetComponentTransform();

			for (FJointData& JointData : MatchPose->BoneData)
			{
				FVector Point = ComponentTransform.TransformPosition(JointData.Position);

				AnimInstanceProxy->AnimDrawDebugSphere(Point, 10.0f, 12.0f, FColor::Yellow, false, -1.0f, 0.5f);
			}

			if(DebugLevel > 1)
			{
				for (int i = 0; i < PoseConfiguration.Num(); ++i)
				{
					float progress = ((float)i) / ((float)PoseConfiguration.Num() - 1);
					FColor Color = (FLinearColor::Blue + progress * (FLinearColor::Red - FLinearColor::Blue)).ToFColor(true);

					FVector LastPoint = FVector::ZeroVector;
					int LastAnimId = -1;
					for (FPoseMatchData& Pose : Poses)
					{
						FVector Point = ComponentTransform.TransformPosition(Pose.BoneData[i].Position);
						

						AnimInstanceProxy->AnimDrawDebugSphere(Point, 3.0f, 6.0f, Color, false, -1.0f, 0.25f);

						if(DebugLevel > 2)
						{
							FVector ArrowPoint = ComponentTransform.TransformVector(Pose.BoneData[i].Velocity) * 0.33333f;
							AnimInstanceProxy->AnimDrawDebugDirectionalArrow(Point, ArrowPoint, 20.0f, Color, false, -1.0f, 0.0f);
						}
						
						if(Pose.AnimId == LastAnimId)
							AnimInstanceProxy->AnimDrawDebugLine(LastPoint, Point, Color, false, -1.0f, 0.0f);

						LastAnimId = Pose.AnimId;
						LastPoint = Point;
					}

				}
			}
		}
	}
#endif
}

#undef LOCTEXT_NAMESPACE