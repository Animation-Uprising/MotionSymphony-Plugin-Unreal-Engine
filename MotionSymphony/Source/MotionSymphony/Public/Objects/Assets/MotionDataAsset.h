// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimComposite.h"
#include "MirroringProfile.h"
#include "Data/PoseMatrixAABB.h"
#include "Utility/KMeansClustering.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "Enumerations/EMMPreProcessEnums.h"
#include "Data/PoseMotionData.h"
#include "Data/CalibrationData.h"
#include "Data/MotionAnimAsset.h"
#include "Objects/Assets/MotionMatchConfig.h"
#include "Data/PoseMatrix.h"
#include "MotionDataAsset.generated.h"

class USkeleton;
class UMotionAnimMetaDataWrapper;
struct FAnimChannelState;

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

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Optimisation")
	UMotionCalibration* PreprocessCalibration;

	UPROPERTY(EditAnywhere, Category = "Motion Matching|Mirroring")
	UMirroringProfile* MirroringProfile;

	/** Has the Motion Data been processed before the last time it's data was changed*/
	UPROPERTY()
	bool bIsProcessed;

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
	
	/** Remaps the pose ID in the pose database to the pose Id in the pose array. This is so that DoNotUse
	 * Poses can be removed from the PoseArray */
	UPROPERTY()
	TArray<int32> PoseIdRemap;

	/** Remaps the pose Id in the pose array to the pose id in the pose database. This is the reverse of
	 * PoseIdRemap so that remaps can be done in both directions */
	UPROPERTY()
	TMap<int32, int32> PoseIdRemapReverse;

	UPROPERTY()
	TMap<FMotionTraitField, FPoseMatrixSection> TraitMatrixMap;

	/**Map of calibration data for normalizing all atoms. This stores the standard deviation of all atoms throughout the data set
	but separates them via motion trait. There is one feature standard deviation per motion trait field. */
	UPROPERTY()
	TMap<FMotionTraitField, FCalibrationData> FeatureStandardDeviations;
	
	/** A list of all poses generated during the pre-process stage. Each pose contains information
	about an animation frame within the animation data set.*/
	UPROPERTY()
	TArray<FPoseMotionData> Poses;
	
	/** The pose matrix, all pose data represented in a single linear array of floats*/
	UPROPERTY() //Todo: Remove Pose Matrix and have it as a raw array
	FPoseMatrix LookupPoseMatrix;

	/** An AABB data structure used to assist with searching through the pose matrix*/
	UPROPERTY(Transient)
	FPoseAABBMatrix PoseAABBMatrix_Outer;

	UPROPERTY(Transient)
	FPoseAABBMatrix PoseAABBMatrix_Inner;
	
	/** The searchable pose matrix, contains only pose data that is searchable with flagged poses removed*/
	UPROPERTY(Transient)
	FPoseMatrix SearchPoseMatrix;
	
#if WITH_EDITORONLY_DATA
	/** A helper for displaying the animation meta data in a MotionDataAsset editor details panel. */
	UPROPERTY()
	UMotionAnimMetaDataWrapper* MotionMetaWrapper;

	/** The index of the Anim currently being previewed.*/
	int32 AnimMetaPreviewIndex;

	EMotionAnimAssetType AnimMetaPreviewType;
#endif

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
	void GenerateSearchPoseMatrix(); //Generates a pose matrix that can be used for searches

	//General
	bool CheckValidForPreProcess() const;
	void PreProcess();
	void ClearPoses();
	bool IsSetupValid();
	bool AreSequencesValid();
	float GetPoseInterval() const;
	float GetPoseFavour(const int32 PoseId) const;
	int32 GetTraitStartIndex(const FMotionTraitField& MotionTrait);
	int32 GetTraitEndIndex(const FMotionTraitField& MotionTrait);
	int32 MatrixPoseIdToDatabasePoseId(int32 MatrixPoseId) const;
	int32 DatabasePoseIdToMatrixPoseId(int32 DatabasePoseId) const;
	bool IsSearchPoseMatrixGenerated() const;
	
	
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

	/** Calculates the Atom count per pose and total pose count and then zero fills the pose matrix to fit all poses*/
	void InitializePoseMatrix();

	void PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror = false);
	void PreProcessBlendSpace(const int32 SourceBlendSpaceIndex, const bool bMirror = false);
	void PreProcessComposite(const int32 SourceCompositeIndex, const bool bMirror = false);
	void GeneratePoseSequencing();
	void MarkEdgePoses(float InMaxAnimBlendTime);
	
};
