// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Data/MotionAnimAsset.h"
#include "CustomAssets/MotionDataAsset.h"
#include "Data/PoseMotionData.h"
#include "TagSection.generated.h"

UCLASS(abstract, editinlinenew, Blueprintable, const, hidecategories = (Object), collapsecategories, meta = (ShowWorldContextPin))
class MOTIONSYMPHONY_API UTagSection : public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()

	/** Tag PreProcess event which is called once for each tag. Use this for tag section specific pre-processing*/
	UFUNCTION(BlueprintImplementableEvent)
	bool Received_PreProcessTag(UPARAM(ref)FMotionAnimAsset& OutMotionAnim, 
		UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) const;

	/** Pose PreProcess event which is called once for each generated pose within a tag area. Use this for pose specific pre-processiong*/
	UFUNCTION(BlueprintImplementableEvent)
	bool Received_PreProcessPose(UPARAM(ref)FPoseMotionData& OutPose, UPARAM(ref)FMotionAnimAsset& OutMotionAnim, 
		UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) const;

	virtual void PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime);
	virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime);

public:	
	virtual void CopyTagData(UTagSection* CopyTag);
};