// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_MotionRecorder.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimInstanceProxy.h"
#include "DrawDebugHelpers.h"

#define LOCTEXT_NAMESPACE "AnimNode_PoseRecorder"

static TAutoConsoleVariable<int32> CVarMotionSnapshotDebug(
	TEXT("a.AnimNode.MoSymph.MotionSnapshot.Debug"),
	0,
	TEXT("Turns Motion Recorder Debugging On / Off.\n")
	TEXT("<=0: Off \n")
	TEXT("  1: On - Minimal\n")
	TEXT("  2: On - Show Velocity\n"));

static TAutoConsoleVariable<int32> CVarMotionSnapshotEnable(
	TEXT("a.AnimNode.MoSymph.MotionSnapshot.Enable"),
	1,
	TEXT("Turns Motion Recorder Node On / Off.\n")
	TEXT("<=0: Off \n")
	TEXT("  1: On\n"));

#if ENGINE_MAJOR_VERSION > 4
IMPLEMENT_ANIMGRAPH_MESSAGE(IMotionSnapper);
const FName IMotionSnapper::Attribute("MotionSnapshot");

class FMotionSnapper : public IMotionSnapper
{
private:
	//Node to target
	FAnimNode_MotionRecorder& Node;

	//Node index
	int32 NodeId;

	//Proxy currently executing
	FAnimInstanceProxy& Proxy;

public:
	FMotionSnapper(const FAnimationBaseContext& InContext, FAnimNode_MotionRecorder* InNode)
		: Node(*InNode),
		NodeId(InContext.GetCurrentNodeId()),
		Proxy(*InContext.AnimInstanceProxy)
	{}

private:
	//IMotionSnapper interface
	virtual struct FAnimNode_MotionRecorder& GetNode() override
	{
		return Node;
	}

	/*virtual FCachedMotionPose& GetMotionPose() override
	{
		return Node.GetMotionPose();
	}

	virtual void RegisterBonesToRecord(TArray<FBoneReference>& BoneReferences) override
	{
		Node.RegisterBonesToRecord(BoneReferences);
	}

	virtual void RegisterBoneIdsToRecord(TArray<int32>& BoneIds) override
	{
		Node.RegisterBoneIdsToRecord(BoneIds);
	}

	virtual void RegisterBoneToRecord(FBoneReference& BoneReference) override
	{
		Node.RegisterBoneToRecord(BoneReference);
	}

	virtual void RegisterBoneToRecord(int32 BoneId) override
	{
		Node.RegisterBoneToRecord(BoneId);
	}

	virtual void ReportBodyVelocity(const FVector& InBodyVelocity) override
	{
		Node.ReportBodyVelocity(InBodyVelocity);
	}*/

	virtual void AddDebugRecord(const FAnimInstanceProxy& InSourceProxy, int32 InSourceNodeId)
	{
#if WITH_EDITORONLY_DATA
		Proxy.RecordNodeAttribute(InSourceProxy, NodeId, InSourceNodeId, IMotionSnapper::Attribute);
#endif
		TRACE_ANIM_NODE_ATTRIBUTE(Proxy, InSourceProxy, NodeId, InSourceNodeId, IMotionSnapper::Attribute);
	}
};
#endif


FAnimNode_MotionRecorder::FAnimNode_MotionRecorder()
	: BodyVelocity(FVector::ZeroVector),
	  BodyVelocityRecordMethod(EBodyVelocityMethod::None),
	  bRetargetPose(true),
	  bVelocityCalcThisFrame(false),
	  bBonesCachedThisFrame(false),
	  AnimInstanceProxy(nullptr)
{
}



FCachedMotionPose& FAnimNode_MotionRecorder::GetMotionPose()
{
	if (!bVelocityCalcThisFrame)
	{
		RecordedPose.CalculateVelocity();
		bVelocityCalcThisFrame = true;
	}

	return RecordedPose;
}

void FAnimNode_MotionRecorder::RegisterBonesToRecord(TArray<FBoneReference>& BoneReferences)
{
	FReferenceSkeleton RefSkeleton = AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();

	int32 CountAdded = 0;
	for (FBoneReference& BoneRef : BoneReferences)
	{
		bool CanAdd = true;
		for (FBoneReference& RecordBoneRef : BonesToRecord)
		{
			if (RecordBoneRef.BoneName == BoneRef.BoneName)
			{
				CanAdd = false;
				break;
			}
		}

		if(CanAdd)
		{
			BonesToRecord.Add(BoneRef);
			++CountAdded;
		}
	}

	if(CountAdded > 0)
	{
		CacheMotionBones();
	}
}

void FAnimNode_MotionRecorder::RegisterBoneIdsToRecord(TArray<int32>& BoneIds)
{
	for (int32 BoneId : BoneIds)
	{
		RecordedPose.CachedBoneData.FindOrAdd(BoneId);
		RecordedPose.MeshToRefSkelMap.FindOrAdd(BoneId) = BoneId;
	}
}

void FAnimNode_MotionRecorder::RegisterBoneToRecord(FBoneReference& BoneReference)
{
	bool CanAdd = true;
	for (FBoneReference& RecordBoneRef : BonesToRecord)
	{
		if (RecordBoneRef.BoneName == BoneReference.BoneName)
		{
			CanAdd = false;
			return;
		}
	}

	BonesToRecord.Add(BoneReference);
	CacheMotionBones();
}

void FAnimNode_MotionRecorder::RegisterBoneToRecord(int32 BoneId)
{
	RecordedPose.CachedBoneData.FindOrAdd(BoneId);
	RecordedPose.MeshToRefSkelMap.FindOrAdd(BoneId) = BoneId;
}

void FAnimNode_MotionRecorder::ReportBodyVelocity(const FVector& InBodyVelocity)
{
	BodyVelocity = InBodyVelocity;
}

void FAnimNode_MotionRecorder::LogRequestError(const FAnimationUpdateContext& Context, const FPoseLinkBase& RequesterPoseLink)
{
#if WITH_EDITORONLY_DATA
	UAnimBlueprint* AnimBlueprint = Context.AnimInstanceProxy->GetAnimBlueprint();
	UAnimBlueprintGeneratedClass* AnimClass = AnimBlueprint ? AnimBlueprint->GetAnimBlueprintGeneratedClass() : nullptr;
	const UObject* RequesterNode = AnimClass ? AnimClass->GetVisualNodeFromNodePropertyIndex(RequesterPoseLink.SourceLinkID) : nullptr;

	FText Message = FText::Format(LOCTEXT("MotionSnapperRequestError", "No Motion Snapper node found for request from '{0}'. Add a motionsnapper node after this request."),
		FText::FromString(GetPathNameSafe(RequesterNode)));
#endif
}

void FAnimNode_MotionRecorder::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread);

	FAnimNode_Base::Initialize_AnyThread(Context);
	Source.Initialize(Context);	

	AnimInstanceProxy = Context.AnimInstanceProxy;
}

void FAnimNode_MotionRecorder::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread);

	FAnimNode_Base::CacheBones_AnyThread(Context);
	Source.CacheBones(Context);
	
	CacheMotionBones();
}

void FAnimNode_MotionRecorder::CacheMotionBones()
{
	if (!AnimInstanceProxy)
	{
		return;
	}

	RecordedPose.MeshToRefSkelMap.Empty(BonesToRecord.Num() + 1);
	RecordedPose.CachedBoneData.Empty(BonesToRecord.Num() + 1);

	const FReferenceSkeleton& RefSkeleton = AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const FBoneContainer& BoneContainer = AnimInstanceProxy->GetRequiredBones();

	//Todo: If the LOD is changing there may be a jump in matching due to 
	for (FBoneReference& BoneRef : BonesToRecord)
	{
		BoneRef.Initialize(BoneContainer);

		if (BoneRef.IsValidToEvaluate())
		{
			RecordedPose.CachedBoneData.FindOrAdd(BoneRef.BoneIndex);
			int32 RefIndex = RefSkeleton.FindBoneIndex(BoneRef.BoneName);
			RecordedPose.MeshToRefSkelMap.FindOrAdd(RefIndex) = BoneRef.BoneIndex;
		}
	}

	bBonesCachedThisFrame = true;
}

void FAnimNode_MotionRecorder::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Update_AnyThread);

#if ENGINE_MAJOR_VERSION > 4
	//Allow nodes further towards the leaves to use the motion snapshot node
	UE::Anim::TScopedGraphMessage<FMotionSnapper> MotionSnapper(Context, Context, this);

	//Handle skipped updates for cached poses by forwarding to motion snapper node in those residual stacks
	/*UE::Anim::TScopedGraphMessage<UE::Anim::FCachedPoseSkippedUpdateHandler> CachedPoseSkippedUpdate(Context, [this, NodeId, &Proxy](TArrayView<const UE::Anim::FMessageStack> InSkippedUpdates)
	{

	}*/

#else
	//Note: The return value of 'Track Ancestor' must be stored in a local variable
	//Otherwise tracking will be null for some reason
	FScopedAnimNodeTracker Tracked = Context.TrackAncestor(this);
#endif

	Source.Update(Context);

	RecordedPose.PoseDeltaTime = Context.GetDeltaTime();

	//Potentially record delta time for predicting pose one frame later?
}

void FAnimNode_MotionRecorder::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread);

	Source.Evaluate(Output);

	FComponentSpacePoseContext CS_Output(Output.AnimInstanceProxy);

	if (bRetargetPose)
	{
		//Create a new retargeted pose, initialize it from our current pose
		FCompactPose RetargetedPose(Output.Pose);

		//Pull the bones out so we can use them directly
		TArray<FTransform> RetargetedToBase;
		RetargetedPose.CopyBonesTo(RetargetedToBase); //The actual current Pose which is additive to the reference pose of the current skeleton 
		const TArray<FTransform>& CompactRefPose = RetargetedPose.GetBoneContainer().GetRefPoseCompactArray();
		
		const TArray<FTransform>& RefSkeletonRefPose = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton().GetRefBonePose();	//The reference pose of the reference skeleton
		const TArray<int>& PoseToSkeletonBoneIndexArray = Output.Pose.GetBoneContainer().GetPoseToSkeletonBoneIndexArray();

		for (int32 i = 0; i < FMath::Min(CompactRefPose.Num(), RetargetedToBase.Num()); ++i)
		{
			FTransform& RetargetBoneTransform = RetargetedToBase[i];											//The actual current bone transform					

			RetargetedToBase[i] = (RetargetedToBase[i] * CompactRefPose[i].Inverse()) * RefSkeletonRefPose[PoseToSkeletonBoneIndexArray[i]];
			RetargetBoneTransform.NormalizeRotation();
		}

		//Set the bones back
		RetargetedPose.CopyBonesFrom(RetargetedToBase);

		//Convert pose to component space
		CS_Output.Pose.InitPose(RetargetedPose);
	}
	else
	{
		//Convert pose to component space
		CS_Output.Pose.InitPose(Output.Pose);
	}
	
	//Record the pose in component space
	RecordedPose.RecordPose(CS_Output.Pose);

	if(bBonesCachedThisFrame)
	{
		RecordedPose.SquashVelocity();
		bBonesCachedThisFrame = false;
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	//const USkeletalMeshComponent* SkelMeshComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
	int32 DebugLevel = CVarMotionSnapshotDebug.GetValueOnAnyThread();

	if (Output.AnimInstanceProxy)
	{
		if (DebugLevel > 0)
		{
			if (DebugLevel > 1 && !bVelocityCalcThisFrame)
			{
				RecordedPose.CalculateVelocity();
				bVelocityCalcThisFrame = true;
			}

			FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
			for (auto& element : RecordedPose.CachedBoneData)
			{
				FVector Point = ComponentTransform.TransformPosition(element.Value.Transform.GetLocation());

				Output.AnimInstanceProxy->AnimDrawDebugSphere(Point,
					10.0f, 15, FColor::Blue, false, -1.0f, 0.0f);

				if (DebugLevel > 1)
				{
					FVector Velocity = ComponentTransform.TransformVector(element.Value.Velocity);
					Output.AnimInstanceProxy->AnimDrawDebugDirectionalArrow(Point, Velocity * 0.3333f, 30.0f, FColor::Blue, false, -1.0f, 0.0f);
				}
			}
		}
	}
#endif

	bVelocityCalcThisFrame = false;
}

void FAnimNode_MotionRecorder::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData);

	const FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	Source.GatherDebugData(DebugData);
}

FCachedMotionPose::FCachedMotionPose()
	: PoseDeltaTime(0.0001f)
{
	CachedBoneData.Empty(6);
}

void FCachedMotionPose::RecordPose(FCSPose<FCompactPose>& Pose)
{
	//FTransform InverseRootTransform = Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0)).Inverse();

	for (auto& element : CachedBoneData)
	{
		element.Value.LastTransform = element.Value.Transform;
		element.Value.Transform = Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(element.Key)) /** InverseRootTransform*/;
	}
}

void FCachedMotionPose::CalculateVelocity()
{
	for (auto& element : CachedBoneData)
	{
		element.Value.Velocity = (element.Value.Transform.GetLocation() - element.Value.LastTransform.GetLocation())
			/ FMath::Max(0.000001f, PoseDeltaTime);
	}
}

void FCachedMotionPose::SquashVelocity()
{
	for (auto& element : CachedBoneData)
	{
		element.Value.LastTransform = element.Value.Transform;
		element.Value.Velocity = FVector::ZeroVector;
	}
}

#undef LOCTEXT_NAMESPACE
