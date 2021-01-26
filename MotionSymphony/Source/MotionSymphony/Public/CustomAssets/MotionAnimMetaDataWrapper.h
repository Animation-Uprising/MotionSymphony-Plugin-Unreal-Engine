#pragma once

#include "CoreMinimal.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "MotionAnimMetaDataWrapper.generated.h"

class UMotionDataAsset;
struct FMotionAnimAsset;

/** This is just a editor only helper so that the IDetailsView can be used to modify
* the motion meta data which is an array of structs. IDetailsView can only be used with UObjects
*/
UCLASS(EditInLineNew, DefaultToInstanced)
class MOTIONSYMPHONY_API UMotionAnimMetaDataWrapper : public UObject
{
	GENERATED_BODY()

public:
	UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer);

	/** Does the animation sequence loop seamlessly? */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	bool bLoop;

	/** Should this animation be used in a mirrored form as well? */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	bool bEnableMirroring;

	/** The favour for all poses in the animation sequence. The pose cost will be multiplied by this for this anim sequence */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	float Favour;

	/** Should the trajectory be flattened so there is no Y value?*/
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	bool bFlattenTrajectory;

	/** The method for pre-processing the past trajectory beyond the limits of the anim sequence */
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	ETrajectoryPreProcessMethod PastTrajectory;

	/** The anim sequence to use for pre-processing motion before the anim sequence if that method is chosen */
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	UAnimSequence* PrecedingMotion;

	/** The method for pre-processing the future trajectory beyond the limits of the anim sequence */
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	ETrajectoryPreProcessMethod FutureTrajectory;

	/** The anim sequence to use for pre-processing motion after the anim sequence if that method is chosen */
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	UAnimSequence* FollowingMotion;

	UMotionDataAsset* ParentAsset;

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual void SetProperties(FMotionAnimAsset* MetaData);
};

UCLASS(EditInLineNew, DefaultToInstanced)
class MOTIONSYMPHONY_API UMotionBlendSpaceMetaDataWrapper : public UMotionAnimMetaDataWrapper
{
	GENERATED_BODY()

public: 
	UPROPERTY(EditAnywhere, Category = "Pre Process")
	FVector2D SampleSpacing;

public:
	UMotionBlendSpaceMetaDataWrapper(const FObjectInitializer& ObjectInitializer);
	virtual void SetProperties(FMotionAnimAsset* MetaData) override;
};
