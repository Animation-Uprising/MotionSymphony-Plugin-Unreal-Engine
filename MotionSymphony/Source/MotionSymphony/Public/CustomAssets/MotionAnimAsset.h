#pragma once

#include "CoreMinimal.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "MotionSymphony.h"
#include "MotionAnimAsset.generated.h"

/** This is data related to animation sequences used in conjunction with Motion Matching. It is additional data
* held externally to the anim sequence as it only relates to Motion Matching and it is used in the FMotionDataAsset
* struct to store this meta data alongside the used animation sequences.
*/
USTRUCT()
struct MOTIONSYMPHONY_API FMotionAnimAsset
{
	GENERATED_USTRUCT_BODY()

public:
	FMotionAnimAsset();
	FMotionAnimAsset(UAnimationAsset* InAnimAsset);

public:
	/**Identifies the type of motion anim asset this is */
	UPROPERTY()
	EMotionAnimAssetType MotionAnimAssetType;

	/** Does the animation sequence loop seamlessly? */
	UPROPERTY()
	bool bLoop;

	/** Should this animation be used in a mirrored form as well? */
	UPROPERTY()
	bool bEnableMirroring;

	/** The favour for all poses in the animation sequence. The pose cost will be multiplied by this for this anim sequence*/
	UPROPERTY()
	float Favour = 1.0f;

	/** Should the trajectory be flattened so there is no Y value?*/
	UPROPERTY()
	bool bFlattenTrajectory = true;

	/** The method for pre-processing the past trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod PastTrajectory;

	/** The method for pre-processing the future trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod FutureTrajectory;

	UPROPERTY()
	UAnimationAsset* AnimAsset;

	/** The anim sequence to use for pre-processing motion before the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* PrecedingMotion;

	/** The anim sequence to use for pre-processing motion after the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* FollowingMotion;

public:
	virtual float GetAnimLength() const;
};

USTRUCT()
struct MOTIONSYMPHONY_API FMotionAnimSequence : public FMotionAnimAsset
{
	GENERATED_USTRUCT_BODY()

public:
	FMotionAnimSequence();
	FMotionAnimSequence(UAnimSequence* InSequence);

public:
	UPROPERTY()
	UAnimSequence* Sequence;

public:
	virtual float GetAnimLength() const override;
};

USTRUCT()
struct MOTIONSYMPHONY_API FMotionBlendSpace : public FMotionAnimAsset
{
	GENERATED_USTRUCT_BODY()

public:
	FMotionBlendSpace();
	FMotionBlendSpace(UBlendSpaceBase* InSequence);

public:
	UPROPERTY()
	UBlendSpaceBase* BlendSpace;

	UPROPERTY()
	FVector2D SampleSpacing;

public:
	virtual float GetAnimLength() const override;
};