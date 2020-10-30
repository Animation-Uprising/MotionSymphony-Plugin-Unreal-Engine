#pragma once

#include "CoreMinimal.h"
#include "EBlendStatus.h"
#include "PoseMotionData.h"
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
	int32 StartPoseId;

	UPROPERTY()
	float StartTime;

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
	float AnimLength;

public:
	float Update(const float DeltaTime, const float BlendTime, const bool bCurrent);

	FAnimChannelState();
	FAnimChannelState(const FPoseMotionData& Pose, EBlendStatus Status, 
		float InWeight, float InAnimLength, bool bInLoop = false, float InTimeOffset=0.0f);
	~FAnimChannelState();
};