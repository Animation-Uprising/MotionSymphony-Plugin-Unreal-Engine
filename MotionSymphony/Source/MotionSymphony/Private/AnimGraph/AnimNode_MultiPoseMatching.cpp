// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_MultiPoseMatching.h"

FAnimNode_MultiPoseMatching::FAnimNode_MultiPoseMatching()
{

}

UAnimSequenceBase* FAnimNode_MultiPoseMatching::FindActiveAnim()
{
	if(Animations.Num() == 0)
		return nullptr;

	int32 AnimId = FMath::Clamp(MatchPose->AnimId, 0, Animations.Num() - 1);

	return Animations[AnimId];
}

#if WITH_EDITOR
void FAnimNode_MultiPoseMatching::PreProcess()
{
	FAnimNode_PoseMatchBase::PreProcess();

	//Find first valid animation
	UAnimSequence* FirstValidSequence = nullptr;
	for (UAnimSequence* CurSequence : Animations)
	{
		if (CurSequence != nullptr)
		{
			FirstValidSequence = CurSequence;
			break;
		}
	}

	if(FirstValidSequence == nullptr)
		return;

	CurrentPose.Empty(PoseConfiguration.Num());
	for (FMatchBone& MatchBone : PoseConfiguration)
	{
		MatchBone.Bone.Initialize(FirstValidSequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	for(int32 i = 0; i < Animations.Num(); ++i)
	{
		UAnimSequence* CurSequence = Animations[i];

		if(CurSequence)
		{
			PreProcessAnimation(CurSequence, i);
		}
	}
}
#endif