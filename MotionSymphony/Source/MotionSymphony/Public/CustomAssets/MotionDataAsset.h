// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimComposite.h"
#include "MirroringProfile.h"
#include "MotionMatchingUtil/KMeansClustering.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "Data/PoseMotionData.h"
#include "Data/CalibrationData.h"
#include "Data/MotionAnimAsset.h"
#include "CustomAssets/MotionMatchConfig.h"
#include "CustomAssets/MMOptimisationModule.h"
#include "Data/DistanceMatchSection.h"
#include "Data/MotionAction.h"
#include "MotionDataAsset.generated.h"

class USkeleton;
class UMotionAnimMetaDataWrapper;
struct FAnimChannelState;


USTRUCT(BlueprintType)
struct MOTIONSYMPHONY_API FDistanceMatchIdentifier
{
	GENERATED_USTRUCT_BODY()

public:
	FDistanceMatchIdentifier();
	FDistanceMatchIdentifier(EDistanceMatchType InMatchType, EDistanceMatchBasis InMatchBasis);
	FDistanceMatchIdentifier(FDistanceMatchSection& InDistanceMatchSection);
	FDistanceMatchIdentifier(const FDistanceMatchSection& InDistanceMatchSection);

	bool operator == (const FDistanceMatchIdentifier rhs) const;

public:
	UPROPERTY()
	EDistanceMatchType MatchType;

	UPROPERTY()
	EDistanceMatchBasis MatchBasis;

};

inline uint16 GetTypeHash(const FDistanceMatchIdentifier A)
{
	return ((uint16)A.MatchBasis << 8) | (uint8)A.MatchType;
}

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FDistanceMatchGroup
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	TArray<FDistanceMatchSection> DistanceMatchSections;
};

/** This is a custom animation asset used for pre-processing and storing motion matching animation data.
 * It is used as the source asset to 'play' with the 'Motion Matching' animation node and is part of the
 * Motion Symphony suite of animation tools.
 */
UCLASS(BlueprintType, HideCategories = ("Animation", "Thumbnail"))
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

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	class UMMOptimisationModule* OptimisationModule;

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

	UPROPERTY()
	TArray<FMotionComposite> SourceComposites;

	/** A list of all poses generated during the pre-process stage. Each pose contains information
	about an animation frame within the animation data set.*/
	UPROPERTY()
	TArray<FPoseMotionData> Poses;

	/**Map of calibration data for normalizing all atoms. This stores the standard deviation of all atoms throughout the data set
	but separates them via motion trait. There is one feature standard deviation per motion trait field. */
	UPROPERTY()
	TMap<FMotionTraitField, FCalibrationData> FeatureStandardDeviations;

	/** A map of distance matching sections that can be searched at runtime to perform distance matching in certain situations */
	UPROPERTY()
	TMap<FDistanceMatchIdentifier, FDistanceMatchGroup> DistanceMatchSections;

	/** An array of action groups. Each action group contains actions for a specific Action Id which is the Id of the group in the list */
	UPROPERTY()
	TArray<FMotionAction> Actions;

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
	int32 GetAnimCount() const;
	int32 GetSourceAnimCount() const;
	int32 GetSourceBlendSpaceCount() const;
	int32 GetSourceCompositeCount() const;
	FMotionAnimAsset* GetSourceAnim(const int32 AnimId, const EMotionAnimAssetType AnimType);
	const FMotionAnimSequence& GetSourceAnimAtIndex(const int32 AnimIndex) const;
	const FMotionBlendSpace& GetSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex) const;
	const FMotionComposite& GetSourceCompositeAtIndex(const int32 CompsoiteIndex) const;
	FMotionAnimSequence& GetEditableSourceAnimAtIndex(const int32 AnimIndex);
	FMotionBlendSpace& GetEditableSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex);
	FMotionComposite& GetEditableSourceCompositeAtIndex(const int32 CompositeIndex);

	void AddSourceAnim(UAnimSequence* AnimSequence);
	void AddSourceBlendSpace(UBlendSpace* BlendSpace);
	void AddSourceComposite(UAnimComposite* Composite);
	bool IsValidSourceAnimIndex(const int32 AnimIndex);
	bool IsValidSourceBlendSpaceIndex(const int32 BlendSpaceIndex);
	bool IsValidSourceCompositeIndex(const int32 CompositeIndex);
	void DeleteSourceAnim(const int32 AnimIndex);
	void DeleteSourceBlendSpace(const int32 BlendSpaceIndex);
	void DeleteSourceComposite(const int32 CompositeIndex);
	void ClearSourceAnims();
	void ClearSourceBlendSpaces();
	void ClearSourceComposites();

	//General
	bool CheckValidForPreProcess() const;
	void PreProcess();
	void ClearPoses();
	bool IsSetupValid();
	bool AreSequencesValid();
	float GetPoseInterval() const;
	bool IsOptimisationValid() const;

	//Distance Matching
	FDistanceMatchGroup& GetDistanceMatchGroup(const EDistanceMatchType MatchType, const EDistanceMatchBasis MatchBasis);
	FDistanceMatchGroup& GetDistanceMatchGroup(const FDistanceMatchIdentifier MatchGroupIdentifier);
	void AddDistanceMatchSection(const FDistanceMatchSection& NewDistanceMatchSection);

	//Actions
	void AddAction(const FPoseMotionData& ClosestPose, const FMotionAnimAsset& MotionAnim, const int32 ActionId, const float Time);

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

	virtual float TickAnimChannelForBlendSpace(const FAnimChannelState& ChannelState,
	                                           FAnimAssetTickContext& Context, TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const;

	virtual float TickAnimChannelForComposite(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context,
		TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const;

	virtual void SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty = true) override;
	virtual USkeletalMesh* GetPreviewMesh(bool bMarkAsDirty = true);
	virtual USkeletalMesh* GetPreviewMesh() const;
	virtual void RefreshParentAssetData();
	virtual float GetMaxCurrentTime();
#if WITH_EDITOR	
	virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive = true);
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap) override;
#endif
	//~ End UAnimationAsset Interface

	void MotionAnimMetaDataModified();
	bool SetAnimMetaPreviewIndex(EMotionAnimAssetType CurAnimType, int32 CurAnimId);

private:
	void AddAnimNotifiesToNotifyQueue(FAnimNotifyQueue& NotifyQueue, TArray<FAnimNotifyEventReference>& Notifies, float InstanceWeight) const;

	void PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror = false);
	void PreProcessBlendSpace(const int32 SourceBlendSpaceIndex, const bool bMirror = false);
	void PreProcessComposite(const int32 SourceCompositeIndex, const bool bMirror = false);
	void GeneratePoseSequencing();
};
