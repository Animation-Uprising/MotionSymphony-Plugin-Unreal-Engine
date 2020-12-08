// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpaceBase.h"
#include "MirroringProfile.h"
#include "MotionMatchingUtil/KMeansClustering.h"
#include "MotionMatchingUtil/PoseLookupTable.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "Data/AnimMirroringData.h"
#include "Data/PoseMotionData.h"
#include "Data/CalibrationData.h"

#include "MotionDataAsset.generated.h"

class USkeleton;


/** This is data related to animation sequences used in conjunction with Motion Matching. It is additional data
* held externally to the anim sequence as it only relates to Motion Matching and it is used in the FMotionDataAsset 
* struct to store this meta data alongside the used animation sequences.
*/
USTRUCT()
struct MOTIONSYMPHONY_API FMotionAnimSequence
{
	GENERATED_USTRUCT_BODY()

public:
	FMotionAnimSequence();
	FMotionAnimSequence(UAnimSequence* InSequence);

public:
	/** Does the animation sequence loop seamlessly? */
	UPROPERTY()
	bool bLoop;

	/** Should this animation be used in a mirrored form as well? */
	UPROPERTY()
	bool bEnableMirroring;

	/** The favour for all poses in the animation sequence. The pose cost will be multiplied by this for this anim sequence*/
	UPROPERTY()
	float Favour = 1.0f;

	/** Placeholder for global tags */
	UPROPERTY()
	int32 GlobalTagId;

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
	UAnimSequence* Sequence;

	/** The anim sequence to use for pre-processing motion before the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* PrecedingMotion;

	/** The anim sequence to use for pre-processing motion after the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* FollowingMotion;
};

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

	/** Placeholder for global tags */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	int32 GlobalTagId;

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
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent);
	void SetProperties(FMotionAnimSequence& MetaData);
};

/** This is a custom animation asset used for pre-processing and storing motion matching animation data.
 * It is used as the source asset to 'play' with the 'Motion Matching' animation node and is part of the
 * Motion Symphony suite of animation tools.
 */
UCLASS()
class MOTIONSYMPHONY_API UMotionDataAsset : public UAnimationAsset
{
	GENERATED_BODY()

public:
	UMotionDataAsset(const FObjectInitializer& ObjectInitializer);
	
public:
	/** The time, in seconds, between each pose recorded in the pre-processing stage (0.05 - 0.1 recommended)*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching", meta = (ClampMin = 0.01f, ClampMax = 0.5f))
	float PoseInterval;
	
	/** The rules for triggering notifies on animations played by the motion matching node*/
	UPROPERTY(EditAnywhere, Category = AnimationNotifies)
	TEnumAsByte<ENotifyTriggerMode::Type> NotifyTriggerMode;

	/** A list of times in the past and future to record trajectory data for each pose. Negative (-ve) values, 
	represent past trajectory points while positive (+ve) values represent future trajectory points. Times should
	be in chronological order from past to future. */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Trajectory Config")
	TArray<float> TrajectoryTimes;

	/** The method to be used for calculating joint velocity. Most of the time this should be left as default */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Pose Config")
	EJointVelocityCalculationMethod JointVelocityCalculationMethod;

	/** The bone Id's of the joints to be recorded in each pose for pose matching. */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Pose Config")
	TArray<int32> PoseJoints;

	/** Check this if the pre-processing should run the optimization algorithm for faster runtime searches. 
	Warning: Optimization can take a lot of time to complete. */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	bool bOptimize;

	/** The number of clusters to create in the first step of trajectory clustering during pre-process*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 KMeansClusterCount;

	/** The number of attempts of KMeans clustering to be performed when choosing a random selection
	of starting cluster points. */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 KMeansAttempts;

	/** The maximum number of K-means clustering iterations for each attempt.*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 KMeansMaxIterations;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 DesiredLookupTableSize = 100;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 MaxLookupColumnSize = 500;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	FCalibrationData PreprocessCalibration;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Mirroring")
	UMirroringProfile* MirroringProfile;

	/** Has the Motion Data been processed before the last time it's data was changed*/
	UPROPERTY()
	bool bIsProcessed;

	/** Has the clustering optimization algorithm been run on the pre-processed animation data?
	If not, the motion matching search will not be optimized*/
	UPROPERTY()
	bool bIsOptimised;

	/** A list of all source animations used for this MotionData asset along with meta data 
	related to the animation sequence for pre-processing and runtime purposes.*/
	UPROPERTY()
	TArray<FMotionAnimSequence> SourceMotionAnims;

	/** A list of all poses generated during the pre-process stage. Each pose contains information
	about an animation frame within the animation data set.*/
	UPROPERTY()
	TArray<FPoseMotionData> Poses;

	/** A list of tag names that were used during pre-processing. This is for lookup only, tags 
	are processed into integers for performance. */
	UPROPERTY()
	TArray<FString> TagIdentifiers; //For RUntime

	/** A lookup table for pose searches. Each pose in the data set points to a single column 
	of this table. At any pose search, only one of these columns will ever be searched. Each
	column holds an Id of a potential successor pose. */
	UPROPERTY()
	FPoseLookupTable PoseLookupTable;

//#if WITH_EDITOR
	/** The final result of the K-Means clustering. This data is only stored if in the editor 
	for the purposes of visual representation and debugging. */
	UPROPERTY()
	FKMeansClusteringSet ChosenTrajClusterSet;

	/** A helper for displaying the animation meta data in a MotionDataAsset editor details panel. */
	UPROPERTY()
	UMotionAnimMetaDataWrapper* MotionMetaWrapper;

	/** The index of the Anim Sequence currently being previewed.*/
	int32 AnimMetaPreviewIndex;
//#endif

public:
	int32 GetSourceAnimCount();
	const FMotionAnimSequence& GetSourceAnimAtIndex(const int32 AnimIndex) const;
	FMotionAnimSequence& GetEditableSourceAnimAtIndex(const int32 AnimIndex);

	void AddSourceAnim(UAnimSequence* AnimSequence);
	bool IsValidSourceAnimIndex(const int32 AnimIndex);
	void DeleteSourceAnim(const int32 AnimIndex);
	void ClearSourceAnims();

	void PreProcess();
	void GeneratePoseCandidateTable();
	void ClearPoses();
	bool IsSetupValid();
	bool AreSequencesValid();

	//Tags
	bool IsTimeTagged(const float RangeTime, const uint8 AtTagIndex, const int32 AtAnimIndex);
	void ResetTagsInAnim(const int32 AnimIndex);
	FString GetTagAtIndex(const int32 TagIndex) const;
	float GetPoseInterval() const;
	int32 GetTagCount();
	int32 GetTagHandle(const FString& InTagName);

	/** UObject Interface*/
	virtual void PostLoad() override;
	/** End UObject Interface*/

	/** UAnimationAsset interface */
	virtual void Serialize(FArchive& Ar) override;
	/** End UAnimationAsset interface */

	//~ Begin UAnimationAsset Interface
#if WITH_EDITOR
	virtual void RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces) override;
#endif
	virtual void TickAssetPlayer(FAnimTickRecord& Instance, struct FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const override;
	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	virtual USkeletalMesh* GetPreviewMesh(bool bMarkAsDirty = true);
	virtual USkeletalMesh* GetPreviewMesh() const;
	virtual void RefreshParentAssetData();
	virtual float GetMaxCurrentTime();
	virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive = true);
	//~ End UAnimationAsset Interface

	void MotionAnimMetaDataModified();
	bool SetAnimMetaPreviewIndex(int32 CurAnimId);

private:
	void PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror = false);
	void GeneratePoseSequencing();
};
