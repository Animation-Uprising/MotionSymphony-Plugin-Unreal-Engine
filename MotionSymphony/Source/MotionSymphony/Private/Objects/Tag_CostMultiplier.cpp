
#include "Objects/Tag_CostMultiplier.h"

UTag_CostMultiplier::UTag_CostMultiplier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


void UTag_CostMultiplier::PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) {}


void UTag_CostMultiplier::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	//TODO: Implement
}
