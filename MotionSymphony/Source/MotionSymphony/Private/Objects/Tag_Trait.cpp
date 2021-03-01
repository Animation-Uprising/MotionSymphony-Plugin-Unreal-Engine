
#include "Objects/Tag_Trait.h"

UTag_Trait::UTag_Trait(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTag_Trait::PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) {}


void UTag_Trait::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	//TODO: Implement
}
