// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatching.h"

FAnimNode_PoseMatching::FAnimNode_PoseMatching()
{
}

UAnimSequenceBase* FAnimNode_PoseMatching::FindActiveAnim()
{
	return Sequence;
}

#if WITH_EDITOR
void FAnimNode_PoseMatching::PreProcess()
{
	if (!Sequence || !Sequence->IsValidToPlay())
	{ 
		return;
	}
	
	FAnimNode_PoseMatchBase::PreProcess();
	
	CurrentPose.Empty(PoseConfig.Num() + 1);
	for (FMatchBone& MatchBone : PoseConfig)
	{
		MatchBone.Bone.Initialize(Sequence->GetSkeleton());
		CurrentPose.Emplace(FJointData());
	}

	//Non mirrored animation
	PreProcessAnimation(Cast<UAnimSequence>(Sequence), 0);

	if (bEnableMirroring && MirroringProfile)
	{
		//Mirrored animation
		PreProcessAnimation(Cast<UAnimSequence>(Sequence), 0, true);
	}
}
#endif