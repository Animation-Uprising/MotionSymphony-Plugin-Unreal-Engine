// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "../Data/JointData.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "AnimNode_MotionRecorder.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_PoseMatchBase.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FPoseMatchData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	int32 PoseId;

	UPROPERTY()
	int32 AnimId;

	UPROPERTY()
	float Time;

	UPROPERTY()
	FVector LocalVelocity;

	UPROPERTY()
	TArray<FJointData> BoneData;

public:
	FPoseMatchData();
	FPoseMatchData(int32 InPoseId, int32 InAnimId, float InTime, FVector& InLocalVelocity);
};

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FMatchBone
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere)
	FBoneReference Bone;

	UPROPERTY(EditAnywhere)
	float PositionWeight;

	UPROPERTY(EditAnywhere)
	float VelocityWeight;

public:
	FMatchBone();
};

USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_PoseMatchBase : public FAnimNode_SequencePlayer
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseMatching, meta = (ClampMin = 0.01f))
	float PoseInterval;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseMatching)
	EPoseMatchMethod PoseMatchMethod;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PoseMatching, meta = (ClampMin = 0.01f))
	float PosesEndTime;

	UPROPERTY(EditAnywhere, Category = PoseCalibration, meta = (ClampMin = 0.0f))
	float BodyVelocityWeight;

	UPROPERTY(EditAnywhere, Category = PoseCalibration)
	TArray<FMatchBone> PoseCalibrationuration;

protected:
	//bool bPreProcessed;
	bool bInitialized;
	bool bInitPoseSearch;

	//Baked poses
	UPROPERTY()
	TArray<FPoseMatchData> Poses;

	//Pose Data extracted from Motion Recorder
	FVector CurrentLocalVelocity;
	TArray<FJointData> CurrentPose;

	//The chosen animation data
	FPoseMatchData* MatchPose;

	FAnimInstanceProxy* AnimInstanceProxy;

public:
	FAnimNode_PoseMatchBase();

#if WITH_EDITOR
	virtual void PreProcess();
#endif

protected:
#if WITH_EDITOR
	virtual void PreProcessAnimation(UAnimSequence* Anim, int32 AnimIndex);
#endif
	virtual void FindMatchPose(const FAnimationUpdateContext& Context); 
	virtual UAnimSequenceBase*	FindActiveAnim();
	void ComputeCurrentPose(const FCachedMotionPose& MotionPose);
	int32 GetMinimaCostPoseId_LQ();
	int32 GetMinimaCostPoseId_HQ();
	int32 GetMinimaCostPoseId_LQ(float& OutCost, int32 StartPose, int32 EndPose);
	int32 GetMinimaCostPoseId_HQ(float& OutCost, int32 StartPose, int32 EndPose);

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	// End of FAnimNode_Base interface
};