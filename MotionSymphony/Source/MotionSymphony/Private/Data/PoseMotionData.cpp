//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "PoseMotionData.h"
#include "MotionSymphony.h"

/**
 * @brief 
 */
FPoseMotionData::FPoseMotionData()
	: PoseId(0),
	AnimType(EMotionAnimAssetType::None),
	BlendSpacePosition(FVector2D(0.0f)),
	NextPoseId(0),
	LastPoseId(0),
	Traits(FMotionTraitField())
{ 	
}

FPoseMotionData::FPoseMotionData(int32 InPoseId, EMotionAnimAssetType InAnimType, int32 InAnimId, float InTime,
	EPoseSearchFlag InPoseFlag, bool bInMirrored, const FMotionTraitField& InTraits)
		:PoseId(InPoseId), 
	  AnimType(InAnimType),
	  AnimId(InAnimId),
	  Time(InTime), 
	  BlendSpacePosition(FVector2D(0.0f)),
	  NextPoseId(InPoseId + 1),
	  LastPoseId(FMath::Clamp(InPoseId - 1, 0, InPoseId)), 
	  bMirrored(bInMirrored),
	  SearchFlag(InPoseFlag), 
	  Traits(InTraits)
{
}

void FPoseMotionData::Clear()
{
	PoseId = -1;
	AnimType = EMotionAnimAssetType::None;
	CandidateSetId = -1;
	AnimId = -1;
	Time = 0;
	BlendSpacePosition = FVector2D(0.0f);
	NextPoseId = -1;
	LastPoseId = -1;
	bMirrored = false;
	SearchFlag = EPoseSearchFlag::Searchable;
	Traits.Clear();
}

// FPoseMotionData& FPoseMotionData::operator+=(const FPoseMotionData& rhs)
// {
// 	//Probably deprecate
// 	Traits |= rhs.Traits;
//
// 	return *this;
// }
//
// FPoseMotionData& FPoseMotionData::operator/=(const float rhs)
// {
// 	//Probably deprecate
// 	return *this;
// }
//
// FPoseMotionData& FPoseMotionData::operator*=(const float rhs)
// {
// 	//Probably deprecate
// 	return *this;
// }