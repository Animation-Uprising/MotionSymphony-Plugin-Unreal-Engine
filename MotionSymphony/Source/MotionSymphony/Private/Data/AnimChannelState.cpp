// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "AnimChannelState.h"
#include "MotionSymphony.h"

FAnimChannelState::FAnimChannelState()
	: Weight(0.0f), 
	  HighestWeight(0.0f), 
	  AnimId(0), 
	  AnimType(EMotionAnimAssetType::None),
	  StartPoseId(0), 
	  StartTime(0.0f), 
	  BlendSpacePosition(FVector2D(0.0f)),
	  Age(0.0f),
	  DecayAge(0.0f), 
	  AnimTime(0.0f), 
	  BlendStatus(EBlendStatus::Inactive),
	  bLoop(false), 
	  bMirrored(false),
	  AnimLength(0.0f)
{ 
}

FAnimChannelState::FAnimChannelState(const FPoseMotionData & InPose, 
	EBlendStatus InBlendStatus, float InWeight, float InAnimLength, 
	bool bInLoop, bool bInMirrored, float InTimeOffset)
	: Weight(InWeight), 
	HighestWeight(InWeight),
	AnimId(InPose.AnimId), 
	AnimType(InPose.AnimType),
	StartPoseId(InPose.PoseId),
	StartTime(InPose.Time),
	BlendSpacePosition(InPose.BlendSpacePosition),
	Age(InTimeOffset), 
	DecayAge(0.0f), 
	AnimTime(InPose.Time + InTimeOffset), 
	BlendStatus(InBlendStatus),
	bLoop(bInLoop),
	bMirrored(bInMirrored),
	AnimLength(InAnimLength)
{ 
	if (Weight > 0.999f)
	{
		StartTime -= 0.3f;
		Age = 0.3f + InTimeOffset;
	}

	if(AnimTime > AnimLength)
		AnimTime = AnimLength;
}

float FAnimChannelState::Update(const float DeltaTime, const float BlendTime, const bool Current)
{
	if (BlendStatus == EBlendStatus::Inactive)
		return false;

	AnimTime += DeltaTime;

	//TODO: AnimTime is used to determine Current Pose. Use a different variable or calculate the actual time
	//of the animation with wrapping when sourcing the animation.
	if (bLoop && AnimTime > AnimLength)
	{
		AnimTime -= AnimLength;
	}

	if (Current)
	{
		Age += DeltaTime;
		Weight = HighestWeight = FMath::Sin((PI / 2.0f) * FMath::Clamp(Age / BlendTime, 0.0f, 1.0f));
	}
	else
	{
		Weight = HighestWeight * (1.0f - FMath::Sin((PI / 2.0f) * FMath::Clamp(DecayAge / BlendTime, 0.0f, 1.0f)));

		if (Weight < DeltaTime)
		{
			Weight = HighestWeight = 0.0f;
			Age = 0.0f;
			DecayAge = 0.0f;
			BlendStatus = EBlendStatus::Inactive;
			return -1.0f;
		}
		else
		{
			Age += DeltaTime;
			DecayAge += DeltaTime;
		}
	}

	return Weight;
}