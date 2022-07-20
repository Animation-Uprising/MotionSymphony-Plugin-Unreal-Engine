// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TagPoint.h"
#include "Tag_ActionMarker.generated.h"

UCLASS(editinlinenew, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTag_ActionMarker : public UTagPoint
{
	GENERATED_UCLASS_BODY()

public:
	/** The name of the action marker for identification purposes */
	UPROPERTY(EditAnywhere, Category = "MotionSymphony|ActionMarker")
	FString ActionName;

public:
	virtual void PreProcessTag(const FPoseMotionData& PointPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float Time) override;
	//virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData) override;

	virtual void CopyTagData(UTagPoint* CopyTag) override;
};