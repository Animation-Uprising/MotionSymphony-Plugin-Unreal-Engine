// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"


#if ENGINE_MAJOR_VERSION > 4
#include "Animation/AnimNodeMessages.h"
#endif

#include "AnimNode_MotionRecorder.generated.h"

UENUM()
enum class EBodyVelocityMethod : uint8
{
	None,
	Manual,
	//VelocityCurve,
	Reported
};

USTRUCT()
struct FCachedMotionBone
{
	GENERATED_BODY()

public:
	FTransform LastTransform;
	FTransform Transform;

	FVector Velocity;
};


USTRUCT()
struct FCachedMotionPose
{
	GENERATED_BODY()

public:
	float PoseDeltaTime;
	TMap<int32, FCachedMotionBone> CachedBoneData;
	TMap<int32, int32> MeshToRefSkelMap;

	FCachedMotionPose();

	void RecordPose(FCSPose<FCompactPose>& Pose);
	void CalculateVelocity();
};

#if ENGINE_MAJOR_VERSION > 4
class MOTIONSYMPHONY_API IMotionSnapper : public UE::Anim::IGraphMessage
{
	DECLARE_ANIMGRAPH_MESSAGE(IMotionSnapper);

public:
	static const FName Attribute;

	virtual struct FAnimNode_MotionRecorder& GetNode() = 0;
	virtual void AddDebugRecord(const FAnimInstanceProxy& InSourceProxy, int32 InSourceNodeId) = 0;
};
#endif


USTRUCT(BlueprintInternalUseOnly)
struct MOTIONSYMPHONY_API FAnimNode_MotionRecorder : public FAnimNode_Base
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink Source;

	UPROPERTY(EditAnywhere, Category = Inputs, meta = (PinHiddenByDefault))
	FVector BodyVelocity;

	UPROPERTY(EditAnywhere, Category = Settings)
	EBodyVelocityMethod BodyVelocityRecordMethod;

	UPROPERTY(EditAnywhere, Category = Settings)
	bool bRetargetPose;

	UPROPERTY(EditAnywhere, Category = BoneReferences)
	TArray<FBoneReference> BonesToRecord;

private:
	bool bVelocityCalcThisFrame;
	bool bBonesCached;

	FCachedMotionPose RecordedPose;

	FAnimInstanceProxy* AnimInstanceProxy;

public:

	FAnimNode_MotionRecorder();

	void CacheMotionBones();
	FCachedMotionPose& GetMotionPose();
	void RegisterBonesToRecord(TArray<FBoneReference>& BoneReferences);
	void RegisterBoneIdsToRecord(TArray<int32>& BoneIds);
	void RegisterBoneToRecord(FBoneReference& BoneReference);
	void RegisterBoneToRecord(int32 BoneId);

	void ReportBodyVelocity(const FVector& InBodyVelocity);

	static void LogRequestError(const FAnimationUpdateContext& Context, const FPoseLinkBase& RequesterPoseLink);

public: 

	// FAnimNode_Base
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base
};