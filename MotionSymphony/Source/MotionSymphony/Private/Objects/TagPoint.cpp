
#include "Objects/TagPoint.h"

UTagPoint::UTagPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTagPoint::PreProcessTag(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	Received_PreProcessTag(OutPose, OutMotionAnim, OutMotionData);
}
