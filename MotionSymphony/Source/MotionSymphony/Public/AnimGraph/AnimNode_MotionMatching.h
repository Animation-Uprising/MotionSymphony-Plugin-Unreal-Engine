// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNode_AssetPlayerBase.h"
#include "AnimNode_MotionRecorder.h"
#include "CustomAssets/MotionDataAsset.h"
#include "CustomAssets/MirroringProfile.h"
#include "Data/Trajectory.h"
#include "Data/TrajectoryPoint.h"
#include "Data/PoseMotionData.h"
#include "Data/AnimChannelState.h"
#include "Data/CalibrationData.h"
#include "Data/AnimMirroringData.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "AnimNode_MotionMatching.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_MotionMatching : public FAnimNode_AssetPlayerBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AlwaysAsPin))
	FTrajectory DesiredTrajectory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
	float UpdateInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (PinShowByDefault, ClampMin = 0.0f))
	float PlaybackRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General", meta = (PinShownByDefault, ClampMin = 0.0f))
	float BlendTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Calibration", meta = (AlwaysAsPin, ClampMin = 0.0f))
	FCalibrationData Calibration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation Data")
	UMotionDataAsset* MotionData;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Animation Mirroring")
	TArray<FBoneMirrorPair> OverrideMirrorPairs;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
	bool bBlendOutEarly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
	EPoseMatchMethod PoseMatchMethod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
	ETransitionMethod TransitionMethod;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Options")
	EPastTrajectoryMode PastTrajectoryMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Blending")
	bool bBlendTrajectory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory Blending")
	float TrajectoryBlendMagnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Favour")
	bool bFavourCurrentPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Favour", meta = (ClampMin = 0.0f))
	float CurrentPoseFavour;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Tolerance Test")
	bool bEnableToleranceTest;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Tolerance Test", meta = (ClampMin = 0.0f))
	float PositionTolerance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pose Tolerance Test", meta = (ClampMin = 0.0f))
	float RotationTolerance;

private:
	float TimeSinceMotionUpdate;
	float TimeSinceMotionChosen;
	float PoseInterpolationValue;
	bool bForcePoseSearch;
	int32 CurrentChosenPoseId;
	int32 DominantBlendChannel; 

	bool bInitialized;
	bool bTriggerTransition;

	FPoseMotionData CurrentInterpolatedPose;
	TArray<FAnimChannelState> BlendChannels;
	FTrajectory ActualTrajectory;

	//Debug
	TArray<int32> HistoricalPosesSearchCounts;
	FAnimInstanceProxy* AnimInstanceProxy; //For Debug drawingR

	FAnimMirroringData MirroringData;

public:
	FAnimNode_MotionMatching();
	~FAnimNode_MotionMatching();


	//FAnimNode_AssetPlayerBase interface
	virtual float GetCurrentAssetTime();
	virtual float GetCurrentAssetTimePlayRateAdjusted();
	virtual float GetCurrentAssetLength();
	virtual UAnimationAsset* GetAnimAsset() override;
	//End of FAnimNode_AssetPlayerBase interface

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void OverrideAsset(UAnimationAsset* NewAsset) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual bool HasPreUpdate() const override;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	// End of FAnimNode_Base interface

private:
	void UpdateBlending(const float DeltaTime);
	void InitializeWithPoseRecorder(const FAnimationUpdateContext& Context);
	void InitializeMatchedTransition(const FAnimationUpdateContext& Context);
	void UpdateMotionMatching(const float DeltaTime, const FAnimationUpdateContext& Context);
	void ComputeCurrentPose();
	void ComputeCurrentPose(const FCachedMotionPose& CachedMotionPose);
	void SchedulePoseSearch(const float DeltaTime, const FAnimationUpdateContext& Context);
	void ScheduleTransitionPoseSearch(const FAnimationUpdateContext& Context);
	int32 GetLowestCostPoseId_LQ();
	int32 GetLowestCostPoseId_LQ(FPoseMotionData& NextPose);
	int32 GetLowestCostPoseId_LQ_Linear(FPoseMotionData& NextPose);
	int32 GetLowestCostPoseId_HQ();
	int32 GetLowestCostPoseId_HQ(FPoseMotionData& NextPose);
	int32 GetLowestCostPoseId_HQ_Linear(FPoseMotionData& NextPose);
	bool NextPoseToleranceTest(FPoseMotionData& NextPose);
	void ApplyTrajectoryBlending();

	void TransitionToPose(int32 PoseId, const FAnimationUpdateContext& Context);
	void JumpToPose(int32 PoseId);
	void BlendToPose(int32 PoseId);

	UAnimSequence* GetAnimAtIndex(const int32 AnimId);
	UAnimSequence* GetPrimaryAnim();
	void EvaluateBlendPose(FPoseContext& Output, const float DeltaTime);
	void CreateTickRecordForNode(const FAnimationUpdateContext& Context, UAnimSequenceBase* Sequence, bool bLooping, float PlayRate);

	void PerformLinearSearchComparrison(int32 ComparePoseId, FPoseMotionData& NextPose);

	void DrawTrajectoryDebug(FAnimInstanceProxy* AnimInstanceProxy);
	void DrawChosenTrajectoryDebug(FAnimInstanceProxy* AnimInstanceProxy);
	void DrawCandidateTrajectories(FPoseCandidateSet& Candidates);
	void DrawPoseTrajectory(FPoseMotionData& Pose, FTransform& CharTransform);
	void DrawSearchCounts();

};