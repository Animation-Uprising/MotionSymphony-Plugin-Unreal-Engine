
#include "Objects/TagSection.h"

UTagSection::UTagSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UTagSection::PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	Received_PreProcessTag(OutMotionAnim, OutMotionData);
}

void UTagSection::PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData)
{
	Received_PreProcessPose(OutPose, OutMotionAnim, OutMotionData);
}