//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_PoseMatching.h"

FAnimNode_PoseMatching::FAnimNode_PoseMatching()
{
}

UAnimSequenceBase* FAnimNode_PoseMatching::FindActiveAnim()
{
	return GetSequence();
}

void FAnimNode_PoseMatching::PreProcess()
{
	const TObjectPtr<UAnimSequence> LocalSequence = Cast<UAnimSequence>(GetSequence());
	if (!LocalSequence)
	{ 
		return;
	}
	
	FAnimNode_PoseMatchBase::PreProcess();

	InitializePoseMatrix(ComputePoseCountForSingleAnimation(LocalSequence));

	if (bEnableMirroring && MirrorDataTable) //Mirrored animation
	{
		PreProcessAnimation(LocalSequence, 0, true);
	}
	else //Only Non Mirrored
	{
		PreProcessAnimation(LocalSequence, 0);
	}
}