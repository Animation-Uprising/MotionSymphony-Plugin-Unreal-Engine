// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "AnimChannelState.h"
#include "MotionSymphony.h"

float FAnimChannelState::Update(const float a_deltaTime, const float a_blendTime, const bool a_current)
{
	if(BlendStatus == EBlendStatus::Inactive)
		return false;

	AnimTime += a_deltaTime;

	//TODO: AnimTime is used to determine current pose. Use a different variable or calculate the actual time
	//of the animation with wrapping when sourcing the animation.
	if (bLoop && AnimTime > AnimLength)
	{
		AnimTime -= AnimLength;
	}
	
	if (a_current)
	{
		Age += a_deltaTime;
		Weight = HighestWeight = FMath::Sin((PI / 2.0f) * FMath::Clamp(Age / a_blendTime, 0.0f, 1.0f));
	}
	else
	{
		Weight = HighestWeight * (1.0f - FMath::Sin((PI / 2.0f) * FMath::Clamp(DecayAge / a_blendTime, 0.0f, 1.0f)));

		if (Weight < a_deltaTime)
		{
			Weight = HighestWeight = 0.0f;
			Age = 0.0f;
			DecayAge = 0.0f;
			BlendStatus = EBlendStatus::Inactive;
			return -1.0f;
		}
		else
		{
			Age += a_deltaTime;
			DecayAge += a_deltaTime;
		}
	}

	return Weight;
}

FAnimChannelState::FAnimChannelState()
	: Weight(0.0f), 
	  HighestWeight(0.0f), 
	  AnimId(0), 
	  StartPoseId(0), 
	  StartTime(0.0f), 
	  Age(0.0f),
	  DecayAge(0.0f), 
	  AnimTime(0.0f), 
	  BlendStatus(EBlendStatus::Inactive),
	  bLoop(false), 
	  AnimLength(0.0f)
{ }

FAnimChannelState::FAnimChannelState(const FPoseMotionData & a_pose, 
	EBlendStatus a_status, float a_weight, float a_animLength, bool a_Loop, float a_timeOffset)
	: Weight(a_weight), 
	HighestWeight(a_weight),
	AnimId(a_pose.AnimId), 
	StartPoseId(a_pose.PoseId),
	StartTime(a_pose.Time),
	Age(a_timeOffset), 
	DecayAge(0.0f), 
	AnimTime(a_pose.Time + a_timeOffset), 
	BlendStatus(a_status),
	bLoop(a_Loop),
	AnimLength(a_animLength)
{ 
	if (Weight > 0.999f)
	{
		StartTime -= 0.3f;
		Age = 0.3f + a_timeOffset;
	}

	if(AnimTime > AnimLength)
		AnimTime = AnimLength;
}

FAnimChannelState::~FAnimChannelState()
{
}