#pragma once

#include "CoreMinimal.h"
#include "PoseMotionData.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "MotionSymphony.h"
#include "AnimChannelState.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FAnimChannelState
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	float Weight;

	UPROPERTY()
	float HighestWeight;
	
	UPROPERTY()
	int32 AnimId;

	UPROPERTY()
	EMotionAnimAssetType AnimType;

	UPROPERTY()
	int32 StartPoseId;

	UPROPERTY()
	float StartTime;

	UPROPERTY()
	FVector2D BlendSpacePosition;

	UPROPERTY()
	float Age;

	UPROPERTY()
	float DecayAge;

	UPROPERTY()
	float AnimTime;

	UPROPERTY()
	EBlendStatus BlendStatus = EBlendStatus::Inactive; //0 == Inactive, 1 == Decay, 2 == Chosen, 3 == Dominant

	UPROPERTY()
	bool bLoop;

	UPROPERTY()
	bool bMirrored;

	UPROPERTY()
	float AnimLength;

	//For Blend Spaces
	TArray<FBlendSampleData> BlendSampleDataCache;

public:
	float Update(const float DeltaTime, const float BlendTime, const bool bCurrent);

	FAnimChannelState();
	FAnimChannelState(const FPoseMotionData& InPose, EBlendStatus InBlendStatus, 
		float InWeight, float InAnimLength, bool bInLoop = false, 
		bool bInMirrored = false, float InTimeOffset=0.0f);
};