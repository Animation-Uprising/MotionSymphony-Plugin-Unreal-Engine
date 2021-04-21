// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_TransitionMatching.h"
#include "AnimGraph/AnimNode_MotionRecorder.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstanceProxy.h"

FTransitionAnimData::FTransitionAnimData()
	: AnimSequence(nullptr),
	CurrentMoveVector(FVector(0.0f)),
	DesiredMoveVector(FVector(0.0f)),
	TransitionDirectionMethod(ETransitionDirectionMethod::RootMotion),
	Favour(1.0f),
	StartPose(0),
	EndPose(0)
{
}

FAnimNode_TransitionMatching::FAnimNode_TransitionMatching()
	: CurrentMoveVector(FVector(0.0f)),
	DesiredMoveVector(FVector(0.0f)),
	DirectionTolerance(30.0f),
	StartDirectionWeight(1.0f),
	EndDirectionWeight(1.0f),
	bUseDistanceMatching(false)
{
}

void FAnimNode_TransitionMatching::FindMatchPose(const FAnimationUpdateContext& Context)
{
	if (Poses.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FAnimNode_TransitionMatching: No poses recorded in node"))
			return;
	}

	if (TransitionAnimData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("FAnimNode_TransitionMatching: No TransitionAnimData, cannot find a pose"))
		return;
	}

	FAnimNode_MotionRecorder* MotionRecorderNode = Context.GetAncestor<FAnimNode_MotionRecorder>();

	if(MotionRecorderNode)
	{
		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());

		int32 MinimaCostPoseId = 0;
		float MinimaCost = 10000000.0f;
		for (FTransitionAnimData& TransitionData : TransitionAnimData)
		{
			float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMoveVector);
			float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMoveVector);

			if (CurrentVectorDelta > DirectionTolerance ||
				DesiredVectorDelta > DirectionTolerance)
			{
				continue;
			}

			//Find the LowestCost Pose from this transition anim data 
			int32 SetMinimaPoseId = -1;
			float SetMinimaCost = 10000000.0f;

			SetMinimaPoseId = GetMinimaCostPoseId(SetMinimaCost, TransitionData.StartPose, TransitionData.EndPose);

			//Add Transition direction cost
			SetMinimaCost += (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight);

			//Apply Favour
			SetMinimaCost *= TransitionData.Favour;

			if (SetMinimaCost < MinimaCost)
			{
				MinimaCost = SetMinimaCost;
				MinimaCostPoseId = SetMinimaPoseId;
			}
		}

		MinimaCostPoseId = FMath::Clamp(MinimaCostPoseId, 0, Poses.Num() - 1);
		MatchPose = &Poses[MinimaCostPoseId];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FAnimNode_PoseMatchBase: Cannot find Motion Snapshot node to pose match against."))

		int32 MinimaTransitionId = 0;
		float MinimaTransitionCost = 10000000.0f;
		for (int32 i = 0; i < TransitionAnimData.Num(); ++i)
		{
			FTransitionAnimData& TransitionData = TransitionAnimData[i];

			float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMoveVector);
			float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMoveVector);

			float Cost = (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight) * TransitionData.Favour;

			if (Cost < MinimaTransitionCost)
			{
				MinimaTransitionCost = Cost;
				MinimaTransitionId = i;
			}
		}

		MinimaTransitionId = FMath::Clamp(MinimaTransitionId, 0, TransitionAnimData.Num() - 1);

		MatchPose = &Poses[TransitionAnimData[MinimaTransitionId].StartPose];
	}

	Sequence = FindActiveAnim();
	InternalTimeAccumulator = StartPosition = MatchPose->Time;
	PlayRateScaleBiasClamp.Reinitialize();
}

UAnimSequenceBase* FAnimNode_TransitionMatching::FindActiveAnim()
{
	if (TransitionAnimData.Num() == 0)
	{
		return nullptr;
	}

	int32 AnimId = FMath::Clamp(MatchPose->AnimId, 0, TransitionAnimData.Num() - 1);

	return TransitionAnimData[AnimId].AnimSequence;
}

#if WITH_EDITOR
void FAnimNode_TransitionMatching::PreProcess()
{
	FAnimNode_PoseMatchBase::PreProcess();

	//Find First valid animation
	FTransitionAnimData* FirstValidTransitionData = nullptr;
	for (FTransitionAnimData& TransitionData : TransitionAnimData)
	{
		if (TransitionData.AnimSequence != nullptr)
		{
			FirstValidTransitionData = &TransitionData;
			break;
		}
	}

	if (FirstValidTransitionData == nullptr)
		return;

	//Initialize Match bone data
	CurrentPose.Empty(PoseConfig.Num());
	for (FMatchBone& MatchBone : PoseConfig)
	{
		MatchBone.Bone.Initialize(FirstValidTransitionData->AnimSequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	for (int32 i = 0; i < TransitionAnimData.Num(); ++i)
	{
		FTransitionAnimData& TransitionData = TransitionAnimData[i];

		if (TransitionData.AnimSequence == nullptr)
			continue;

		TransitionData.StartPose = Poses.Num();

		PreProcessAnimation(TransitionData.AnimSequence, i);

		if (TransitionData.TransitionDirectionMethod == ETransitionDirectionMethod::RootMotion)
		{
			if (TransitionData.AnimSequence->HasRootMotion())
			{
				float SequneceLength = TransitionData.AnimSequence->SequenceLength;
				float HalfSequenceLength = SequneceLength * 0.5f;

				FTransform StartRootMotion = TransitionData.AnimSequence->ExtractRootMotion(0.0f, 0.05f, false);
				TransitionData.CurrentMoveVector = StartRootMotion.GetLocation().GetSafeNormal();

				FTransform RootMotion = TransitionData.AnimSequence->ExtractRootMotion(HalfSequenceLength, HalfSequenceLength, false);
				TransitionData.DesiredMoveVector = RootMotion.GetLocation().GetSafeNormal();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimNode_TransitionMatching: Attempting to extract transition direction from root motion, but this sequence does not contain root motion"));
			}
		}

		TransitionData.EndPose = Poses.Num() - 1;
	}
}
#endif