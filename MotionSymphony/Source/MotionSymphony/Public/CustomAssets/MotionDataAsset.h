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
#include "CustomAssets/MotionAnimAsset.h"
#include "CustomAssets/MotionMatchConfig.h"
#include "CustomAssets/MotionCalibration.h"
#include "MotionDataAsset.generated.h"

class USkeleton;
class UMotionAnimMetaDataWrapper;
struct FAnimChannelState;

/** This is a custom animation asset used for pre-processing and storing motion matching animation data.
 * It is used as the source asset to 'play' with the 'Motion Matching' animation node and is part of the
 * Motion Symphony suite of animation tools.
 */
UCLASS(HideCategories = ("Animation"))
class MOTIONSYMPHONY_API UMotionDataAsset : public UAnimationAsset
{
	GENERATED_BODY()

public:
	UMotionDataAsset(const FObjectInitializer& ObjectInitializer);
	
public:
	/** The time, in seconds, between each pose recorded in the pre-processing stage (0.05 - 0.1 recommended)*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching", meta = (ClampMin = 0.01f, ClampMax = 0.5f))
	float PoseInterval;

	/** The configuration to use for this motion data asset. This includes the skeleton, trajectory points and 
	pose joints to match when pre-processing and at runtime. Use the same configuration for this asset as you
	do on the runtime node.*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	UMotionMatchConfig* MotionMatchConfig;

	/** The method to be used for calculating joint velocity. Most of the time this should be left as default */
	UPROPERTY(EditAnywhere, Category = "Motion Matching")
	EJointVelocityCalculationMethod JointVelocityCalculationMethod;

	/** The rules for triggering notifies on animations played by the motion matching node*/
	UPROPERTY(EditAnywhere, Category = AnimationNotifies)
	TEnumAsByte<ENotifyTriggerMode::Type> NotifyTriggerMode;

	/** Check this if the pre-processing should run the optimization algorithm for faster runtime searches. 
	Warning: Optimization can take a lot of time to complete. */
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	bool bOptimize;

	/** The number of clusters to create in the first step of trajectory clustering during pre-process*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 KMeansClusterCount;

	/** The maximum number of K-means clustering iterations for each attempt.*/
	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 KMeansMaxIterations;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 DesiredLookupTableSize = 100;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation", meta = (ClampMin = 1))
	int32 MaxLookupColumnSize = 500;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	UMotionCalibration* PreprocessCalibration;

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

	/** A list of all source blend spaces used for this Motion Data asset along with meta data 
	related to the blend space for pre-processing and runtime purposes*/
	UPROPERTY()
	TArray<FMotionBlendSpace> SourceBlendSpaces;

	/** A list of all poses generated during the pre-process stage. Each pose contains information
	about an animation frame within the animation data set.*/
	UPROPERTY()
	TArray<FPoseMotionData> Poses;

	UPROPERTY()
	FCalibrationData FeatureStandardDeviations;

	/** A list of tag names that were used during pre-processing. This is for lookup only, tags 
	are processed into integers for performance. */
	UPROPERTY()
	TMap<uint64, FString> TagIdentifiers; //For Runtime

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

	/** The index of the Anim currently being previewed.*/
	int32 AnimMetaPreviewIndex;

	EMotionAnimAssetType AnimMetaPreviewType;
//#endif

public:
	//Anim Assets
	int32 GetSourceAnimCount();
	int32 GetSourceBlendSpaceCount();
	FMotionAnimAsset* GetSourceAnim(const int32 AnimId, const EMotionAnimAssetType AnimType);
	const FMotionAnimSequence& GetSourceAnimAtIndex(const int32 AnimIndex) const;
	const FMotionBlendSpace& GetSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex) const;
	FMotionAnimSequence& GetEditableSourceAnimAtIndex(const int32 AnimIndex);
	FMotionBlendSpace& GetEditableSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex);

	void AddSourceAnim(UAnimSequence* AnimSequence);
	void AddSourceBlendSpace(UBlendSpaceBase* BlendSpace);
	bool IsValidSourceAnimIndex(const int32 AnimIndex);
	bool IsValidSourceBlendSpaceIndex(const int32 BlendSpaceIndex);
	void DeleteSourceAnim(const int32 AnimIndex);
	void DeleteSourceBlendSpace(const int32 BlendSpaceIndex);
	void ClearSourceAnims();
	void ClearSourceBlendSpaces();

	//General
	void PreProcess();
	void GeneratePoseCandidateTable();
	void ClearPoses();
	bool IsSetupValid();
	bool AreSequencesValid();
	float GetPoseInterval() const;

	//Tags
	uint64 FindOrCreateTags(const TArray<FString>& InTagNames);
	bool IsTimeTagged(const float RangeTime, const uint64 AtTagIndex, const int32 AtAnimIndex);
	void ResetTagsInAnim(const int32 AnimIndex);
	int32 GetTagCount();
	uint64 GetTagHandle(const FString& InTagName);
	FString GetTagName(const uint64 TagIndex) const;

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

	virtual float TickAnimChannelForSequence(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context,
		TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const;

	virtual float TickAnimChannelForBlendSpace(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context,
		TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const;

	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	virtual USkeletalMesh* GetPreviewMesh(bool bMarkAsDirty = true);
	virtual USkeletalMesh* GetPreviewMesh() const;
	virtual void RefreshParentAssetData();
	virtual float GetMaxCurrentTime();
	virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive = true);
	//~ End UAnimationAsset Interface

	void MotionAnimMetaDataModified();
	bool SetAnimMetaPreviewIndex(EMotionAnimAssetType CurAnimType, int32 CurAnimId);

private:
	void AddAnimNotifiesToNotifyQueue(FAnimNotifyQueue& NotifyQueue, TArray<FAnimNotifyEventReference>& Notifies, float InstanceWeight) const;

	void PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror = false);
	void PreProcessBlendSpace(const int32 SourceBlendSpaceIndex, const bool bMirror = false);
	void GeneratePoseSequencing();
};
