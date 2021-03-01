
#include "Objects/Tag_DoNotUse.h"

UTag_DoNotUse::UTag_DoNotUse(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTag_DoNotUse::PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) {}

void UTag_DoNotUse::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	OutPose.bDoNotUse = true;
}