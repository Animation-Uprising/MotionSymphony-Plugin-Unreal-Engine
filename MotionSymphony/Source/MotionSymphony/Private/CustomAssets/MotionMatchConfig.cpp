// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MotionMatchConfig.h"

#define LOCTEXT_NAMESPACE "MotionMatchConfig"

UMotionMatchConfig::UMotionMatchConfig(const FObjectInitializer& ObjectInitializer)
	: SourceSkeleton(nullptr),
	UpAxis(EAllAxis::Z),
	ForwardAxis(EAllAxis::Y)
{
}

void UMotionMatchConfig::Initialize()
{
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("MotionMatchConfig: Trying to initialize bone references but there is no source skeleton set. Please set a skeleton on your motion match configuration before using it"));
	}

	for(UMatchFeatureBase* MatchFeature : Features)
	{
		if(MatchFeature)
		{
			MatchFeature->Initialize();
		}
	}

	ComputeOffsets();
}

void UMotionMatchConfig::ComputeOffsets()
{
	Offsets.Empty(Features.Num() + 1);

	TotalDimensionCount = 0;
	ResponseDimensionCount = 0;
	QualityDimensionCount = 0;
	for(const TObjectPtr<UMatchFeatureBase> Feature : Features)
	{
		Offsets.Add(TotalDimensionCount);
		const int32 FeatureSize = Feature->Size();
		
		if(Feature->PoseCategory == EPoseCategory::Quality)
		{
			QualityDimensionCount += Feature->Size();
		}
		else
		{
			ResponseDimensionCount += Feature->Size();
		}

		TotalDimensionCount += FeatureSize;
	}
}

USkeleton* UMotionMatchConfig::GetSkeleton(bool& bInvalidSkeletonIsError)
{
	return SourceSkeleton;
}

USkeleton* UMotionMatchConfig::GetSourceSkeleton() const
{
	return SourceSkeleton;
}

void UMotionMatchConfig::SetSourceSkeleton(USkeleton* Skeleton)
{
	SourceSkeleton = Skeleton;
}

bool UMotionMatchConfig::IsSetupValid()
{
	bool bIsValid = true;

	//Check that a source skeleton is set
	if (!SourceSkeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config: validity check failed. Source skeleton is not set (null)."));
		bIsValid = false;
	}

	int32 QualityFeatureCount = 0;
	int32 ResponseFeatureCount = 0;
	for(const UMatchFeatureBase* Feature : Features)
	{
		if(Feature)
		{
			if(Feature->PoseCategory == EPoseCategory::Quality)
			{
				++QualityFeatureCount;
			}
			else
			{
				++ResponseFeatureCount;
			}
			
			if(!Feature->IsSetupValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Match Match Config: Validity check failed. One of the match features is not valid"));
				bIsValid = false;
				break;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Motion Match Config: Validity check failed. There is an invalid (null) match feature present."));
			bIsValid = false;
			break;
		}
	}

	//Check that there is at least one quality match feature present
	if(QualityFeatureCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config: Valididty Check failed. There must be at least one quality feature in the motion config feature set."));
		bIsValid = false;
	}

	//Check that there is at least one response match feature present
	if(ResponseFeatureCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Match Config: Validity check failed. THere must be at least one response feature in the motion config feature set."));
		bIsValid = false;
	}

	return bIsValid;
}

int32 UMotionMatchConfig::ComputeResponseArraySize()
{
	int32 Count = 0;
	for(const TObjectPtr<UMatchFeatureBase> MatchFeaturePtr : Features)
	{
		if(MatchFeaturePtr->PoseCategory == EPoseCategory::Responsiveness)
		{
			Count += MatchFeaturePtr->Size();
		}
	}

	return Count;
}

int32 UMotionMatchConfig::ComputeQualityArraySize()
{
	int32 Count = 0;
	for(const TObjectPtr<UMatchFeatureBase> MatchFeaturePtr : Features)
	{
		if(MatchFeaturePtr->PoseCategory == EPoseCategory::Quality)
		{
			Count += MatchFeaturePtr->Size();
		}
	}

	return Count;
}

#undef LOCTEXT_NAMESPACE
