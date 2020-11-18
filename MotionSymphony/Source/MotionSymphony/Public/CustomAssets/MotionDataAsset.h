// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpaceBase.h"
#include "MotionMatchingUtil/KMeansClustering.h"
#include "MotionMatchingUtil/PoseLookupTable.h"
#include "Enumerations/EJointVelocityCalculationMethod.h"
#include "Enumerations/ETrajectoryPreProcessMethod.h"
#include "Data/PoseMotionData.h"
#include "Data/CalibrationData.h"
#include "MotionDataAsset.generated.h"

class USkeleton;


/** This is data related to animation sequences used in conjunction with Motion Matching. It is additional data
* held externally to the anim sequence as it only relates to Motion Matching and it is used in the FMotionDataAsset 
* struct to store this meta data alongside the used animation sequences.
*/
USTRUCT()
struct MOTIONSYMPHONY_API FMotionAnimMetaData
{
	GENERATED_USTRUCT_BODY()

public:
	/** Does the animation sequence loop seamlessly? */
	UPROPERTY()
	bool bLoop;

	/** The favour for all poses in the animation sequence. The pose cost will be multiplied by this for this anim sequence*/
	UPROPERTY()
	float Favour = 1.0f;

	/** Placeholder for global tags */
	UPROPERTY()
	int32 GlobalTagId;

#if WITH_EDITORONLY_DATA
	/** Should the trajectory be flattened so there is no Y value?*/
	UPROPERTY()
	bool bFlattenTrajectory = true;

	/** The method for pre-processing the past trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod PastTrajectory;

	/** The method for pre-processing the future trajectory beyond the limits of the anim sequence */
	UPROPERTY()
	ETrajectoryPreProcessMethod FutureTrajectory;

	/** The anim sequence to use for pre-processing motion before the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* PrecedingMotion;

	/** The anim sequence to use for pre-processing motion after the anim sequence if that method is chosen */
	UPROPERTY()
	UAnimSequence* FollowingMotion;
#endif
};

/** This is just a editor only helper so that the IDetailsView can be used to modify 
* the motion meta data which is an array of structs. IDetailsView can only be used with UObjects 
*/
#if WITH_EDITOR 
UCLASS(EditInLineNew, DefaultToInstanced)
class MOTIONSYMPHONY_API UMotionAnimMetaDataWrapper : public UObject
{
	GENERATED_BODY()

public:
	UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer);

	/** Does the animation sequence loop seamlessly? */
	UPROPERTY(EditAnywhere, Category = "Runtime")
	bool bLoop;

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
	void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
	void SetProperties(FMotionAnimMetaData& MetaData);
};
#endif

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

	/** */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 0.5f, ClampMax = 1.0f))
	float CandidateSimilarityThreshold;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 DesiredLookupTableSize = 100;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 MaxLookupColumnSize = 500;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	FCalibrationData PreprocessCalibration;

	/** Has the Motion Data been processed before the last time it's data was changed*/
	UPROPERTY()
	bool bIsProcessed;

	/** A list of all the source animations used for this MotionData asset*/
	UPROPERTY()
	TArray<UAnimSequence*> SourceAnimations;

	/** A list of all the additional motion matching meta data associated with the source animations
	and set in the MotionDataAsset editor.*/
	UPROPERTY()
	TArray<FMotionAnimMetaData> SourceAnimMetaData;

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

#if WITH_EDITORONLY_DATA
	/** The final result of the K-Means clustering. This data is only stored if in the editor 
	for the purposes of visual representation and debugging. */
	UPROPERTY()
	FKMeansClusteringSet ChosenTrajClusterSet;

	/** A helper for displaying the animation meta data in a MotionDataAsset editor details panel. */
	UPROPERTY()
	UMotionAnimMetaDataWrapper* MotionMetaWrapper;

	/** The index of the Anim Sequence currently being previewed.*/
	int32 AnimMetaPreviewIndex;
#endif

public:
	int32 GetSourceAnimCount();
	UAnimSequence* GetSourceAnimAtIndex(const int32 animIndex) const;
	FMotionAnimMetaData* GetSourceAnimMetaAtIndex(const int32 animIndex);
	/*void SetSourceSkeleton(USkeleton* skeleton);*/

	void AddSourceAnim(UAnimSequence* animSequence);
	bool IsValidSourceAnimIndex(const int32 animIndex);
	void DeleteSourceAnim(const int32 animIndex);
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
	virtual void RefreshParentAssetData() override;
	virtual float GetMaxCurrentTime();
	virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive = true) override;
	//~ End UAnimationAsset Interface

#if WITH_EDITOR
	void MotionAnimMetaDataModified();
	bool SetAnimMetaPreviewIndex(int32 CurAnimId);
#endif

private:
	void PreProcessAnim(const int32 a_sourceAnimIndex);
	void GeneratePoseSequencing();
};
