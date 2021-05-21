// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/MotionTraitField.h"
#include "MotionAction.generated.h"

/** Data strucutre payload for informing Motion Matching node of a desired action */
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FMotionActionPayload
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MotionSymphony|Actions")
	int32 ActionId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MotionSymphony|Actions")
	float LeadLength;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "MotionSymphony|Actions")
	float TailLength;

public:
	FMotionActionPayload();
};

/** Data structure for defining a single action marker */
USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FMotionAction
{
	GENERATED_USTRUCT_BODY()

public:
	/** The id of this action, unique within it's action group */
	UPROPERTY()
	int32 ActionId;

	/** The Id of the closest pose to this action marker*/
	UPROPERTY()
	int32 PoseId;

	/** The exact time of the action marker*/
	UPROPERTY()
	float Time;

	/** The trait associated with this Action */
	UPROPERTY()
	FMotionTraitField Trait;

	/** Id of the last action in the action group. -1 if the last action is from a different animation or if there is no prior action marker */
	UPROPERTY()
	int32 LastActionId;

	/** Id of the next action in the action group. -1 if the next action is from a different animation or if there is no 'next action marker */
	UPROPERTY()
	int32 NextActionId;

public:
	FMotionAction();
	FMotionAction(int32 InActionId, int32 InPoseId, float InTime,
		FMotionTraitField InTrait, int32 InLastActionId, int32 InNextActionId);
};