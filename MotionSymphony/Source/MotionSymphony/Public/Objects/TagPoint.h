#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CustomAssets/MotionAnimAsset.h"
#include "CustomAssets/MotionDataAsset.h"
#include "Data/PoseMotionData.h"
#include "TagPoint.generated.h"

UCLASS(abstract, Blueprintable, const, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTagPoint : public UAnimNotify
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintImplementableEvent)
	bool Received_PreProcessTag(UPARAM(ref)FPoseMotionData& OutPose, UPARAM(ref)FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) const;

	virtual void PreProcessTag(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData);
};