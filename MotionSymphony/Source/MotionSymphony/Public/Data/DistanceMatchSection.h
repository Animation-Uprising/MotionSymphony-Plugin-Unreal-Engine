// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Enumerations/EDistanceMatchingEnums.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Animation/AnimCurveTypes.h"
#include "DistanceMatchSection.generated.h"

USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FDistanceMatchPayload
{
	GENERATED_USTRUCT_BODY()

public:
	FDistanceMatchPayload();
	FDistanceMatchPayload(const bool bInTrigger, const EDistanceMatchType InMatchType, 
		const EDistanceMatchBasis InMatchBasis, const float InMarkerDistance);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|DistanceMatching")
	bool bTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|DistanceMatching")
	EDistanceMatchType MatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|DistanceMatching")
	EDistanceMatchBasis MatchBasis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionSymphony|DistanceMatching")
	float MarkerDistance; //Can be positional or rotational distance
};

/** A data structure for storing 'distance matching' animation data for the motion matching node*/
USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FDistanceMatchSection
{
	GENERATED_USTRUCT_BODY()

public:
	FDistanceMatchSection();
	FDistanceMatchSection(const float InTargetTime, const EDistanceMatchType InMatchType, 
		const EDistanceMatchBasis InMatchBasis, const int32 InAnimId, const EMotionAnimAssetType InAnimType, 
		const int32 InStartPoseId, const int32 InEndPoseId, const float InStartTime, const float InEndTime);

	virtual ~FDistanceMatchSection();

	virtual void GenerateDistanceCurve(const UAnimSequence* Sequence);
	virtual void GenerateRotationCurve(const UAnimSequence* Sequence);

	float FindMatchingTime(const float DesiredDistance, int32& StartKey);

public:
	/** The time of the target for distance matching. i.e. the point at which the animation is stopped*/
	UPROPERTY()
	float TargetTime;

	UPROPERTY()
	float StartTime;

	UPROPERTY()
	float EndTime;

	/** The type of distance matching. Forward = matching towards a stop, backward = matching from being stopped, both = plant or change direction */
	UPROPERTY()
	EDistanceMatchType MatchType;

	/** Is the distance matching done via position or rotation*/
	UPROPERTY()
	EDistanceMatchBasis MatchBasis;

	/** The animation id that this distance match section exists on*/
	UPROPERTY()
	int32 AnimId;

	/** The type of animation that this distance match section exists on */
	UPROPERTY()
	EMotionAnimAssetType AnimType;

	/** The pose at the start of the distance match range */
	UPROPERTY()
	int32 StartPoseId;

	/** The pose at the end of the distance match range */
	UPROPERTY()
	int32 EndPoseId;

	/** The maximum distance value on the distance curve */
	UPROPERTY()
	float MaxDistance;

	UPROPERTY()
	FFloatCurve DistanceCurve;
};