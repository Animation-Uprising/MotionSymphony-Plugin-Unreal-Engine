// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_TransitionMatching.h"
#include "AnimGraph/AnimNode_MotionRecorder.h"
#include "AnimationRuntime.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstanceProxy.h"

FTransitionAnimData::FTransitionAnimData()
	: AnimSequence(nullptr),
	CurrentMove(FVector(0.0f)),
	DesiredMove(FVector(0.0f)),
	TransitionDirectionMethod(ETransitionDirectionMethod::Manual),
	CostMultiplier(1.0f),
	bMirror(false),
	StartPose(0),
	EndPose(0)
{
}

FTransitionAnimData::FTransitionAnimData(const FTransitionAnimData& CopyTransition, bool bInMirror /*= false*/)
	: AnimSequence(CopyTransition.AnimSequence),
	CurrentMove(CopyTransition.CurrentMove),
	DesiredMove(CopyTransition.DesiredMove),
	TransitionDirectionMethod(CopyTransition.TransitionDirectionMethod),
	CostMultiplier(CopyTransition.CostMultiplier),
	bMirror(bInMirror),
	StartPose(CopyTransition.StartPose),
	EndPose(CopyTransition.EndPose)
{
	if (bInMirror)
	{
		StartPose = 0;
		EndPose = 0;
		CurrentMove.X *= -1;
		DesiredMove.X *= -1;
	}
}

FAnimNode_TransitionMatching::FAnimNode_TransitionMatching()
	: CurrentMoveVector(FVector(0.0f)),
	DesiredMoveVector(FVector(0.0f)),
	StartDirectionWeight(1.0f),
	EndDirectionWeight(1.0f)
	//bUseDistanceMatching(false)
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
		switch (TransitionMatchingOrder)
		{
			case ETransitionMatchingOrder::TransitionPriority: MinimaCostPoseId = GetMinimaCostPoseId_TransitionPriority(); break;
			case ETransitionMatchingOrder::PoseAndTransitionCombined: MinimaCostPoseId = GetMinimaCostPoseId_PoseTransitionWeighted(); break;
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

			float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMove);
			float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMove);

			float Cost = (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight) * TransitionData.CostMultiplier;

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

int32 FAnimNode_TransitionMatching::GetMinimaCostPoseId_TransitionPriority()
{
	//First find out which transition set is the best match
	FTransitionAnimData* MinimaCostSet = nullptr;
	int32 MinimaCostPoseId = 0;
	float MinimaCost = 10000000.0f;
	bool bMinimaSetMirrored = false;
	for(FTransitionAnimData& TransitionData : TransitionAnimData)
	{
		float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMove);
		float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMove);

		float SetCost = (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight);

		SetCost *= TransitionData.CostMultiplier;

		if (SetCost < MinimaCost)
		{
			MinimaCost = SetCost;
			MinimaCostSet = &TransitionData;
		}
	}

	//Search mirrored transitions as well if mirroring is enabled
	if(bEnableMirroring)
	{
		for (FTransitionAnimData& TransitionData : MirroredTransitionAnimData)
		{
			float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMove);
			float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMove);

			float SetCost = (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight);

			SetCost *= TransitionData.CostMultiplier;

			if (SetCost < MinimaCost)
			{
				MinimaCost = SetCost;
				MinimaCostSet = &TransitionData;
			}
		}
	}

	//Within the chosen transition (MinimaCostSet) Find the best pose to match to
	float Cost = 0.0f;
	return GetMinimaCostPoseId(Cost, MinimaCostSet->StartPose, MinimaCostSet->EndPose);
}

int32 FAnimNode_TransitionMatching::GetMinimaCostPoseId_PoseTransitionWeighted()
{
	int32 MinimaCostPoseId = 0;
	float MinimaCost = 10000000.0f;
	for (FTransitionAnimData& TransitionData : TransitionAnimData)
	{
		float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMove);
		float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMove);

		//Find the Lowest Cost Pose from this transition anim data 
		int32 SetMinimaPoseId = -1;
		float SetMinimaCost = 10000000.0f;

		SetMinimaPoseId = GetMinimaCostPoseId(SetMinimaCost, TransitionData.StartPose, TransitionData.EndPose);

		//Add Transition direction cost
		SetMinimaCost += (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight);

		//Apply CostMultiplier
		SetMinimaCost *= TransitionData.CostMultiplier;

		if (SetMinimaCost < MinimaCost)
		{
			MinimaCost = SetMinimaCost;
			MinimaCostPoseId = SetMinimaPoseId;
		}
	}

	if(bEnableMirroring)
	{
		for (FTransitionAnimData& TransitionData : MirroredTransitionAnimData)
		{
			float CurrentVectorDelta = FVector::DistSquared(CurrentMoveVector, TransitionData.CurrentMove);
			float DesiredVectorDelta = FVector::DistSquared(DesiredMoveVector, TransitionData.DesiredMove);

			//Find the Lowest Cost Pose from this transition anim data 
			int32 SetMinimaPoseId = -1;
			float SetMinimaCost = 10000000.0f;

			SetMinimaPoseId = GetMinimaCostPoseId(SetMinimaCost, TransitionData.StartPose, TransitionData.EndPose);

			//Add Transition direction cost
			SetMinimaCost += (CurrentVectorDelta * StartDirectionWeight) + (DesiredVectorDelta * EndDirectionWeight);

			//Apply CostMultiplier
			SetMinimaCost *= TransitionData.CostMultiplier;

			if (SetMinimaCost < MinimaCost)
			{
				MinimaCost = SetMinimaCost;
				MinimaCostPoseId = SetMinimaPoseId;
			}
		}
	}

	return MinimaCostPoseId;
}

int32 FAnimNode_TransitionMatching::GetAnimationIndex(UAnimSequence* AnimSequence)
{
	if(!AnimSequence)
	{
		return 0;
	}

	for (int32 i = 0; i < TransitionAnimData.Num(); ++i)
	{
		if (AnimSequence == TransitionAnimData[i].AnimSequence)
		{
			return i;
		}
	}

	return 0;
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
				float SequenceLength = TransitionData.AnimSequence->SequenceLength;

				FTransform StartRootMotion = TransitionData.AnimSequence->ExtractRootMotion(0.0f, 0.05f, false);
				TransitionData.CurrentMove = StartRootMotion.GetLocation().GetSafeNormal();

				FTransform RootMotion = TransitionData.AnimSequence->ExtractRootMotion(0.0f, SequenceLength, false);
				TransitionData.DesiredMove = RootMotion.GetLocation().GetSafeNormal();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AnimNode_TransitionMatching: Attempting to extract transition direction from root motion, but this sequence does not contain root motion"));
			}
		}

		TransitionData.EndPose = Poses.Num() - 1;

		//Copy the transition data for mirroring if mirroring is enabled for this transition
		if (bEnableMirroring && TransitionData.bMirror)
		{
			MirroredTransitionAnimData.Emplace(FTransitionAnimData(TransitionData, true));
		}
	}

	//Pre-Process Mirroring
	if (bEnableMirroring)
	{
		for (int32 i = 0; i < MirroredTransitionAnimData.Num(); ++i)
		{
			FTransitionAnimData& TransitionData = MirroredTransitionAnimData[i];

			if (TransitionData.AnimSequence == nullptr)
				continue;

			TransitionData.StartPose = Poses.Num();

			PreProcessAnimation(TransitionData.AnimSequence, GetAnimationIndex(TransitionData.AnimSequence), true);

			TransitionData.EndPose = Poses.Num() - 1;
		}
	}
}
#endif