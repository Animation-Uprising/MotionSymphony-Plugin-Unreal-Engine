// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TagSection.h"
#include "Data/MotionTraitField.h"
#include "Tag_Trait.generated.h"

UCLASS(editinlinenew, hidecategories = Object, collapsecategories)
class MOTIONSYMPHONY_API UTag_Trait : public UTagSection
{
	GENERATED_UCLASS_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Tag")
	FString TraitName;

	FMotionTraitField TraitHandle;

public:
	virtual void PreProcessTag(FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) override;
	virtual void PreProcessPose(FPoseMotionData& OutPose, FMotionAnimAsset& OutMotionAnim, UMotionDataAsset* OutMotionData, const float StartTime, const float EndTime) override;

	virtual void CopyTagData(UTagSection* CopyTag) override;
};