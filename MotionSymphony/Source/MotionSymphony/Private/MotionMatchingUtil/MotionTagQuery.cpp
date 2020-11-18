// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "MotionMatchingUtil/MotionTagQuery.h"

FMotionTagQuery::FMotionTagQuery()
{
	for (int i = 0; i < MOTION_TAG_COUNT; ++i)
	{
		AllSetTags[i] = true;
		AllUnsetTags[i] = false;
	}
}

FMotionTagQuery::FMotionTagQuery(UMotionDataAsset* InMotionData)
{
	SetTargetMotionData(InMotionData);

	for (int i = 0; i < MOTION_TAG_COUNT; ++i)
	{
		AllSetTags[i] = true;
		AllUnsetTags[i] = false;
	}
}

void FMotionTagQuery::SetTargetMotionData(UMotionDataAsset* InMotionData)
{
	if (!InMotionData)
		return;

	TargetMotionData = InMotionData;

	TagLimit = FMath::Min(MOTION_TAG_COUNT, TargetMotionData->TagIdentifiers.Num());
}

bool FMotionTagQuery::IsValid()
{
	if (!TargetMotionData)
		return false;

	int32 tagCount = TargetMotionData->TagIdentifiers.Num();

	if (IncludeTags.Num() != tagCount
		|| ExcludeTags.Num() != tagCount)
	{
		return false;
	}


	return true;
}

void FMotionTagQuery::Has(int32 TagHandle)
{
	if (TagHandle > -1 && TagHandle < TagLimit)
	{
		IncludeTags[TagHandle] = true;
		ExcludeTags[TagHandle] = false;
	}
	//TODO: Debug Log if its not in range ?
}

void FMotionTagQuery::Has(FString& TagName)
{
	Has(TargetMotionData->GetTagHandle(TagName));
}

void FMotionTagQuery::Has(TStaticBitArray<MOTION_TAG_COUNT>& TagHandles)
{
	IncludeTags |= TagHandles;

	TStaticBitArray<MOTION_TAG_COUNT> bitsToRemove = ExcludeTags;
	bitsToRemove &= TagHandles;
	ExcludeTags ^= bitsToRemove;
}

void FMotionTagQuery::Not(int32 TagHandle)
{
	if (TagHandle > -1 && TagHandle < TagLimit)
	{
		IncludeTags[TagHandle] = false;
		ExcludeTags[TagHandle] = true;
	}
	//TODO: Debug Log if its not in range ?
}

void FMotionTagQuery::Not(FString TagName)
{
	Not(TargetMotionData->GetTagHandle(TagName));
}

void FMotionTagQuery::Not(TStaticBitArray<MOTION_TAG_COUNT>& TagHandles)
{
	ExcludeTags |= TagHandles;

	TStaticBitArray<MOTION_TAG_COUNT> bitsToRemove = IncludeTags;
	bitsToRemove &= TagHandles;
	IncludeTags ^= bitsToRemove;
}

void FMotionTagQuery::AndNothingElse()
{
	ExcludeTags = AllSetTags;
	ExcludeTags ^= IncludeTags;
}

void FMotionTagQuery::Invert()
{

	TStaticBitArray<MOTION_TAG_COUNT> cacheIncludeTags = IncludeTags;

	IncludeTags = ExcludeTags;
	ExcludeTags = cacheIncludeTags;
}

void FMotionTagQuery::ClearIncludeTags()
{
	IncludeTags = AllUnsetTags;
}

void FMotionTagQuery::ClearExcludeTags()
{
	ExcludeTags = AllUnsetTags;
}

void FMotionTagQuery::ClearAllTags()
{
	IncludeTags = AllUnsetTags;
	ExcludeTags = AllUnsetTags;
}