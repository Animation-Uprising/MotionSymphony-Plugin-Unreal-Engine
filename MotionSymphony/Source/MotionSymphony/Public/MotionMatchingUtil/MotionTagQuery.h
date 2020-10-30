// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/BitArray.h"
#include "Containers/StaticBitArray.h"
#include "MotionDataAsset.h"
#include "MotionTagQuery.generated.h"

#define MOTION_TAG_COUNT 128

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FMotionTagQuery
{
	GENERATED_USTRUCT_BODY()

public:

	TStaticBitArray<MOTION_TAG_COUNT> IncludeTags;
	TStaticBitArray<MOTION_TAG_COUNT> ExcludeTags;

private:
	UMotionDataAsset* TargetMotionData;

	TStaticBitArray<MOTION_TAG_COUNT> AllSetTags;
	TStaticBitArray<MOTION_TAG_COUNT> AllUnsetTags;

	int32 TagLimit;

public:
	FMotionTagQuery();
	FMotionTagQuery(UMotionDataAsset* InMotionData);

	void SetTargetMotionData(UMotionDataAsset* InMotionData);

	bool IsValid();

	void Has(int32 TagHandle);
	void Has(FString& TagName);
	void Has(TStaticBitArray<MOTION_TAG_COUNT>& TagHandles);

	void Not(int32 TagHandle);
	void Not(FString TagName);
	void Not(TStaticBitArray<MOTION_TAG_COUNT>& TagHandles);

	void AndNothingElse();
	void Invert();
	
	void ClearIncludeTags();
	void ClearExcludeTags();
	void ClearAllTags();
};