#pragma once

#include "CoreMinimal.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "Enumerations/EDistanceMatchingEnums.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "MotionAnimAsset.generated.h"

/** This is data related to animation sequences used in conjunction with Motion Matching. It is additional data
* held externally to the anim sequence as it only relates to Motion Matching and it is used in the FMotionDataAsset
* struct to store this meta data alongside the used animation sequences.
*/
USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FMotionAnimAsset
{
	GENERATED_BODY()

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

	/** Should the trajectory be flattened so there is no Y value?*/
	UPROPERTY()
	bool bFlattenTrajectory = true;

	/** The method for pre-processing the past trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod PastTrajectory;

	/** The method for pre-processing the future trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod FutureTrajectory;

	/** The actual animation asset referenced for this MotionAnimAsset. i.e. AnimSequence or BlendSpace */
	UPROPERTY()
	UAnimationAsset* AnimAsset;

	/** The anim sequence to use for pre-processing motion before the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* PrecedingMotion;

	/** The anim sequence to use for pre-processing motion after the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* FollowingMotion;

	/** Can this animation use distance matching. If so which type: Backward, Forward or both?*/
	UPROPERTY()
	EDistanceMatchType DistanceMatchType;

	/** The basis for distance matching. Positional or Rotational distance?*/
	UPROPERTY()
	EDistanceMatchBasis DistanceMatchBasis;

	/** A cost multiplier for all poses in the animation sequence. The pose cost will be multiplied by this for this anim sequence*/
	UPROPERTY()
	float CostMultiplier = 1.0f;

	/** A list of tag names to be applied to this entire animation. Tags will be converted to enum ID's on a pose level.*/
	UPROPERTY()
	TArray<FString> TagNames;

	UPROPERTY()
	TArray<struct FAnimNotifyEvent> Tags;

	class UMotionDataAsset* ParentMotionDataAsset;

#if WITH_EDITORONLY_DATA
	// if you change Notifies array, this will need to be rebuilt
	UPROPERTY()
	TArray<FAnimNotifyTrack> MotionTagTracks;

#endif // WITH_EDITORONLY_DATA


public:
	virtual ~FMotionAnimAsset();

	virtual double GetAnimLength() const;
	virtual double GetFrameRate() const;
	virtual int32 GetTickResolution() const;

	virtual void PostLoad();

	void SortTags();
	bool RemoveTags(const TArray<FName> & TagsToRemove);

	void GetMotionTags(const float& StartTime, const float& DeltaTime, const bool bAllowLooping, TArray<FAnimNotifyEventReference>& OutActiveNotifies) const;
	virtual void GetMotionTagsFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, TArray<FAnimNotifyEventReference>& OutActiveNotifies) const;

	void InitializeTagTrack();
	void ClampTagAtEndOfSequence();

	//uint8* FindTagPropertyData(int32 TagIndex, FArrayProperty*& ArrayProperty);
	
	virtual void RefreshCacheData();

#if WITH_EDITOR
protected:
	DECLARE_MULTICAST_DELEGATE(FOnTagChangedMulticaster);
	FOnTagChangedMulticaster OnTagChanged;

public:
	typedef FOnTagChangedMulticaster::FDelegate FOnTagChanged;

	void RegisterOnTagChanged(const FOnTagChanged& Delegate);
	void UnRegisterOnTagChanged(void* Unregister);
#endif

	// return true if anim Tag is available 
	virtual bool IsTagAvailable() const;
};

USTRUCT()
struct MOTIONSYMPHONY_API FMotionAnimSequence : public FMotionAnimAsset
{
	GENERATED_BODY()

public:
	FMotionAnimSequence();
	FMotionAnimSequence(UAnimSequence* InSequence);

public:
	UPROPERTY()
	UAnimSequence* Sequence;

public:
	virtual ~FMotionAnimSequence();

	virtual double GetAnimLength() const override;
	virtual double GetFrameRate() const override;
};

USTRUCT()
struct MOTIONSYMPHONY_API FMotionBlendSpace : public FMotionAnimAsset
{
	GENERATED_BODY()

public:
	FMotionBlendSpace();
	FMotionBlendSpace(class UBlendSpaceBase* InSequence);

public:
	UPROPERTY()
	UBlendSpaceBase* BlendSpace;

	UPROPERTY()
	FVector2D SampleSpacing;

public:
	virtual ~FMotionBlendSpace();

	virtual double GetAnimLength() const override;
	virtual double GetFrameRate() const override;
};