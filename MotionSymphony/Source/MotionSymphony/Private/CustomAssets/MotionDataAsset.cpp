// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "MotionDataAsset.h"
#include "MMPreProcessUtils.h"
#include "AnimChannelState.h"
#include "Animation/AnimNotifyQueue.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

#if WITH_EDITOR
UMotionAnimMetaDataWrapper::UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer)
	: bLoop(false), 
	Favour(1.0f), 
	GlobalTagId(0), 
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	PrecedingMotion(nullptr),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges), 
	FollowingMotion(nullptr), 
	ParentAsset(nullptr)
{
}
#endif

void UMotionAnimMetaDataWrapper::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	if (ParentAsset == nullptr)
		return;

	ParentAsset->MotionAnimMetaDataModified();
}

UMotionDataAsset::UMotionDataAsset(const FObjectInitializer& ObjectInitializer)
	: PoseInterval(0.1f), 
	JointVelocityCalculationMethod(EJointVelocityCalculationMethod::BodyDependent),
	bIsProcessed(false),
#if WITH_EDITOR	
	MotionMetaWrapper(nullptr), 
	AnimMetaPreviewIndex(-1)
#endif
{
#if(WITH_EDITOR)
	MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
	MotionMetaWrapper->ParentAsset = this;
#endif
}

#if WITH_EDITOR
void UMotionAnimMetaDataWrapper::SetProperties(FMotionAnimMetaData& MetaData)
{
	Modify();
	bLoop = MetaData.bLoop;
	Favour = MetaData.Favour;
	bFlattenTrajectory = MetaData.bFlattenTrajectory;
	PrecedingMotion = MetaData.PrecedingMotion;
	GlobalTagId = MetaData.GlobalTagId;
	FutureTrajectory = MetaData.FutureTrajectory;
	PastTrajectory = MetaData.PastTrajectory;
	MarkPackageDirty();
}
#endif

int32 UMotionDataAsset::GetSourceAnimCount()
{
	return SourceAnimations.Num();
}

UAnimSequence* UMotionDataAsset::GetSourceAnimAtIndex(const int32 AnimIndex) const
{
	if(AnimIndex < 0 || AnimIndex > SourceAnimations.Num())
		return nullptr;

	return SourceAnimations[AnimIndex];
}

FMotionAnimMetaData* UMotionDataAsset::GetSourceAnimMetaAtIndex(const int32 AnimIndex)
{
	if (AnimIndex < 0 || AnimIndex > SourceAnimMetaData.Num())
		return nullptr;

	return &SourceAnimMetaData[AnimIndex];
}

//void UMotionDataAsset::SetSourceSkeleton(USkeleton* skeleton)
//{
//	//Todo: cross check skeleton with existing animations. Ask
//	// the user what they want to do about conflicts
//
//	Modify();
//	SetSkeleton(skeleton);
//	MarkPackageDirty();
//
//	bIsProcessed = false;
//}

void UMotionDataAsset::AddSourceAnim(UAnimSequence* AnimSequence)
{
	if (!AnimSequence)
		return;

	//Todo:: Check if AnimSequence is valid. Maybe an dialog warning

	Modify();
	SourceAnimations.Add(AnimSequence);
	SourceAnimMetaData.Emplace(FMotionAnimMetaData());
	MarkPackageDirty();

	bIsProcessed = false;
}

bool UMotionDataAsset::IsValidSourceAnimIndex(const int32 AnimIndex)
{
	return SourceAnimations.IsValidIndex(AnimIndex);
}

void UMotionDataAsset::DeleteSourceAnim(const int32 AnimIndex)
{
	if (AnimIndex < 0 || AnimIndex >= SourceAnimations.Num())
		return;

	Modify();
	SourceAnimations.RemoveAt(AnimIndex);
	SourceAnimMetaData.RemoveAt(AnimIndex);
	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	if (AnimIndex < AnimMetaPreviewIndex)
		SetAnimMetaPreviewIndex(AnimMetaPreviewIndex - 1);
#endif
}

void UMotionDataAsset::ClearSourceAnims()
{
	Modify();
	SourceAnimations.Empty(SourceAnimations.Num());
	SourceAnimMetaData.Empty(SourceAnimMetaData.Num());
	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	SourceAnimMetaData.Empty(SourceAnimMetaData.Num());
	AnimMetaPreviewIndex = -1;
#endif
}

void UMotionDataAsset::PreProcess()
{
	FScopedSlowTask MMPreProcessTask(3, LOCTEXT("Motion Matching PreProcessor", "Pre-Processing..."));
	MMPreProcessTask.MakeDialog();

	FScopedSlowTask MMPreAnimAnalyseTask(SourceAnimations.Num(), LOCTEXT("Motion Matching PreProcessor", "Analyzing Animation Poses"));
	MMPreAnimAnalyseTask.MakeDialog();

	if(!IsSetupValid())
		return;

	ClearPoses();

	MMPreProcessTask.EnterProgressFrame();

	for (int32 i = 0; i < SourceAnimations.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();
		PreProcessAnim(i);
	}

	GeneratePoseSequencing();

	MMPreProcessTask.EnterProgressFrame();

	GeneratePoseCandidateTable();
	bIsProcessed = true;

	MMPreProcessTask.EnterProgressFrame();
}

void UMotionDataAsset::GeneratePoseCandidateTable()
{
	FScopedSlowTask KMeansAttemptsTask(KMeansAttempts, LOCTEXT("Motion Matching Optimization", "KMeans Clustering"));
	KMeansAttemptsTask.MakeDialog();

	//Step 1: Do Trajectory clustering a number of times based on KMeansAttempts
	TArray<FKMeansClusteringSet> KMCS;
	KMCS.Empty(KMeansAttempts + 1);
	for (int32 i = 0; i < KMeansAttempts; ++i)
	{
		
		KMCS.Emplace(FKMeansClusteringSet());
		KMCS[i].BeginClustering(Poses, KMeansClusterCount, KMeansMaxIterations, false);
		KMeansAttemptsTask.EnterProgressFrame();
	}

	FScopedSlowTask KMeansVarianceTask(KMCS.Num(), LOCTEXT("Motion Matching Optimization", "KMeans Variance"));
	KMeansVarianceTask.MakeDialog();

	//Step 2: Find the best KMeans clustering attempt based on variance
	float LowestClusterVariance = 20000000.0f;
	int32 BestClusterSetId = -1;
	for (int32 i = 0; i < KMCS.Num(); ++i)
	{
		float Variance = KMCS[i].CalculateVariance();

		if (Variance < LowestClusterVariance)
		{
			LowestClusterVariance = Variance;
			BestClusterSetId = i;
		}

		KMeansVarianceTask.EnterProgressFrame();
	}

	ChosenTrajClusterSet = KMCS[BestClusterSetId];
	KMCS.Empty();

	//Step 3: Create a lookup table with each pose as a Key
	PoseLookupTable.Process(Poses, ChosenTrajClusterSet, CandidateSimilarityThreshold, 
		DesiredLookupTableSize, MaxLookupColumnSize);
}

void UMotionDataAsset::ClearPoses()
{
	Poses.Empty();
	bIsProcessed = false;
}

bool UMotionDataAsset::IsSetupValid()
{
	if (!GetSkeleton())
		return false;

	if(SourceAnimations.Num() == 0)
		return false;

	if(PoseJoints.Num() == 0)
		return false;

	if(TrajectoryTimes.Num() == 0)
		return false;

	return true;
}

bool UMotionDataAsset::AreSequencesValid()
{
	for (UAnimSequence* SourceSequence : SourceAnimations)
	{
		if (SourceSequence == nullptr 
			|| !SourceSequence->IsValidToPlay()
			|| SourceSequence->IsValidAdditive())
		{
			return false;
		}
	}

	return true;
}

bool UMotionDataAsset::IsTimeTagged(const float RangeTime, const uint8 AtTagIndex, const int32 AtAnimIndex)
{
	//TODO: Implement
	return false;
}

void UMotionDataAsset::ResetTagsInAnim(const int32 AnimIndex)
{
	//TODO: Implement
}

FString UMotionDataAsset::GetTagAtIndex(const int32 TagIndex) const
{
	if (TagIdentifiers.IsValidIndex(TagIndex))
	{
		return TagIdentifiers[TagIndex];
	}

	return FString();
}

float UMotionDataAsset::GetPoseInterval() const
{
	return PoseInterval;
}

int32 UMotionDataAsset::GetTagCount()
{
	return TagIdentifiers.Num();
}

int32 UMotionDataAsset::GetTagHandle(const FString& InTagName)
{
	for (int32 i = 0; i < TagIdentifiers.Num(); ++i)
	{
		if (InTagName == TagIdentifiers[i])
		{
			return i;
		}
	}

	return -1;
}

void UMotionDataAsset::PostLoad()
{
	Super::Super::PostLoad();
}

void UMotionDataAsset::Serialize(FArchive& Ar)
{
	Super::Super::Serialize(Ar);
}

#if WITH_EDITOR
void UMotionDataAsset::RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces)
{
	//Todo: Editor Only, implement later
}
#endif

void UMotionDataAsset::TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const
{
	const float DeltaTime = Context.GetDeltaTime();
	const bool bGenerateNotifies = NotifyTriggerMode != ENotifyTriggerMode::None;

	TArray<FAnimNotifyEventReference> Notifies;
	
	const TArray<FAnimChannelState>* BlendChannels = reinterpret_cast<TArray<FAnimChannelState>*>(Instance.BlendSpace.BlendSampleDataCache);

	float HighestWeight = -10000000.0f;
	int32 HighestWeightChannelId = -1;
	for (int32 i = 0; i < BlendChannels->Num(); ++i)
	{
		const FAnimChannelState& ChannelState = (*BlendChannels)[i];

		if (ChannelState.BlendStatus != EBlendStatus::Inactive
			&& ChannelState.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			UAnimSequence* Sequence = GetSourceAnimAtIndex(ChannelState.AnimId);
			bool bLooping = SourceAnimMetaData[ChannelState.AnimId].bLoop;
			
			if (Sequence)
			{
				const float& CurrentSampleDataTime = ChannelState.AnimTime;
				const float CurrentTime = FMath::Clamp(ChannelState.AnimTime, 0.0f, Sequence->SequenceLength);
				const float PreviousTime = CurrentSampleDataTime - DeltaTime;

				if (bGenerateNotifies)
				{
					if(NotifyTriggerMode == ENotifyTriggerMode::AllAnimations)
					{
						Sequence->GetAnimNotifies(PreviousTime, DeltaTime, bLooping, Notifies);
					}
					else
					{
						if(ChannelState.Weight > HighestWeight)
						{
							HighestWeight = ChannelState.Weight;
							HighestWeightChannelId = i;
						}
					}
				}

				//Root Motion
				if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything && Sequence->bEnableRootMotion)
				{
					Context.RootMotionMovementParams.AccumulateWithBlend(Sequence->ExtractRootMotion(
						PreviousTime, DeltaTime, bLooping), ChannelState.Weight);
				}

				UE_LOG(LogAnimation, Verbose, TEXT("%d. Blending animation(%s) with %f weight at time %0.2f"), i + 1, *Sequence->GetName(), ChannelState.Weight, CurrentTime);
			}
		}
	}


	if (bGenerateNotifies)
	{
		if (NotifyTriggerMode == ENotifyTriggerMode::HighestWeightedAnimation)
		{
			const FAnimChannelState& ChannelState = (*BlendChannels)[HighestWeightChannelId];
			UAnimSequence* Sequence = GetSourceAnimAtIndex(ChannelState.AnimId);
			const float PreviousTime = ChannelState.AnimTime - DeltaTime;
			bool bLooping = SourceAnimMetaData[ChannelState.AnimId].bLoop;

			Sequence->GetAnimNotifies(PreviousTime, DeltaTime, bLooping, Notifies);
		}

		if(Notifies.Num() > 0)
		{
			//NotifyQueue.AddAnimNotifies(true, Notifies, Instance.EffectiveBlendWeight);
		}
	}
}

void UMotionDataAsset::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty /*= true*/)
{
//#if WITH_EDITORONLY_DATA
//	if (bMarkAsDirty)
//	{
//		Modify();
//	}
//	PreviewSkeletalMesh = PreviewMesh;
//#endif
}

USkeletalMesh* UMotionDataAsset::GetPreviewMesh(bool bMarkAsDirty /*= true*/)
{
	return nullptr;
}

USkeletalMesh* UMotionDataAsset::GetPreviewMesh() const
{
//#if WITH_EDITORONLY_DATA
//	if (!PreviewSkeletalMesh.IsValid())
//	{
//		PreviewSkeletalMesh.LoadSynchronous();
//	}
//	return PreviewSkeletalMesh.Get();
//#else
	return nullptr;
//#endif
}

void UMotionDataAsset::RefreshParentAssetData()
{

}

float UMotionDataAsset::GetMaxCurrentTime()
{
	return 1.0f;
}

bool UMotionDataAsset::GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive)
{
	for (UAnimSequence* Sequence : SourceAnimations)
	{
		if (Sequence != nullptr)
		{
			AnimationSequences.Add(Sequence);
		}
	}

	return AnimationSequences.Num() > 0;
}


#if(WITH_EDITOR)
void UMotionDataAsset::MotionAnimMetaDataModified()
{
	if (MotionMetaWrapper == nullptr
		|| AnimMetaPreviewIndex < 0
		|| AnimMetaPreviewIndex >= SourceAnimMetaData.Num())
	{
		return;
	}

	FMotionAnimMetaData& metaData = SourceAnimMetaData[AnimMetaPreviewIndex];

	Modify();
	metaData.bLoop = MotionMetaWrapper->bLoop;
	metaData.Favour = MotionMetaWrapper->Favour;
	metaData.bFlattenTrajectory = MotionMetaWrapper->bFlattenTrajectory;
	metaData.GlobalTagId = MotionMetaWrapper->GlobalTagId;
	metaData.PastTrajectory = MotionMetaWrapper->PastTrajectory;
	metaData.FutureTrajectory = MotionMetaWrapper->FutureTrajectory;
	metaData.PrecedingMotion = MotionMetaWrapper->PrecedingMotion;
	metaData.FollowingMotion = MotionMetaWrapper->FollowingMotion;
	MarkPackageDirty();
}


bool UMotionDataAsset::SetAnimMetaPreviewIndex(int32 CurAnimId)
{
	AnimMetaPreviewIndex = CurAnimId;

	if (MotionMetaWrapper == nullptr)
	{
		MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
		MotionMetaWrapper->ParentAsset = this;
	}

	if (CurAnimId < 0 || CurAnimId >= SourceAnimMetaData.Num())
		return false;

	MotionMetaWrapper->SetProperties(SourceAnimMetaData[AnimMetaPreviewIndex]);

	return true;
}
#endif

void UMotionDataAsset::PreProcessAnim(const int32 SourceAnimIndex)
{
	//UE_LOG(LogTemp, Warning, TEXT("Anim Processed"));

	UAnimSequence* Anim = SourceAnimations[SourceAnimIndex];
	const FMotionAnimMetaData* AnimData = &SourceAnimMetaData[SourceAnimIndex];

	if (!Anim)
		return;

	Modify();

	const float AnimLength = Anim->SequenceLength;
	float CurrentTime = 0.0f;
	float TimeHorizon = TrajectoryTimes.Last();

	if(PoseInterval < 0.01f)
		PoseInterval = 0.05f;

	while (CurrentTime <= AnimLength)
	{
		int32 PoseId = Poses.Num();
		//Todo: implement a less naive assessment of DoNotUse checking
		bool DoNotUse = (CurrentTime < TimeHorizon || CurrentTime > AnimLength - TimeHorizon) ? true : false;

		FVector RootVelocity;
		FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, Anim, CurrentTime, PoseInterval);

		float PoseFavour = AnimData->Favour; //Todo: Multiply by individual pose favour from tag tracks

		FPoseMotionData NewPoseData = FPoseMotionData(PoseId, SourceAnimIndex, CurrentTime, PoseFavour, DoNotUse, RootVelocity);
		
		//Process trajectory for pose
		for (int32 i = 0; i < TrajectoryTimes.Num(); ++i)
		{
			FTrajectoryPoint Point;

			if (AnimData->bLoop)
			{
				FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(Point, Anim, CurrentTime, TrajectoryTimes[i]);
			}
			else
			{
				float PointTime = TrajectoryTimes[i];

				if (PointTime < 0.0f)
				{
					//past Point
					FMMPreProcessUtils::ExtractPastTrajectoryPoint(Point, Anim, CurrentTime, PointTime,
						AnimData->PastTrajectory, AnimData->PrecedingMotion);
				}
				else
				{
					FMMPreProcessUtils::ExtractFutureTrajectoryPoint(Point, Anim, CurrentTime, PointTime,
						AnimData->FutureTrajectory, AnimData->FollowingMotion);
				}
			}
		
			if (AnimData->bFlattenTrajectory)
			{
				Point.Position.Z = 0.0f;
			}

			NewPoseData.Trajectory.Add(Point);
		}

		//Process joints for pose
		for (int32 i = 0; i < PoseJoints.Num(); ++i)
		{
			FJointData JointData;
			FMMPreProcessUtils::ExtractJointData(JointData, Anim, PoseJoints[i], CurrentTime, PoseInterval);
			NewPoseData.JointData.Add(JointData);
		}

		NewPoseData.DoNotUse = FMMPreProcessUtils::GetDoNotUseTag(Anim, CurrentTime, PoseInterval);
		
		Poses.Add(NewPoseData);
		CurrentTime += PoseInterval;
	}
}

void UMotionDataAsset::GeneratePoseSequencing()
{
	for (int32 i = 0; i < Poses.Num(); ++i)
	{
		FPoseMotionData& Pose = Poses[i];
		FPoseMotionData& BeforePose = Poses[FMath::Max(0, i - 1)];
		FPoseMotionData& AfterPose = Poses[FMath::Min(i + 1, Poses.Num() - 1)];

		if (BeforePose.AnimId == Pose.AnimId)
		{
			Pose.LastPoseId = BeforePose.PoseId;
		}
		else
		{
			const FMotionAnimMetaData* AnimData = GetSourceAnimMetaAtIndex(Pose.AnimId);
			const UAnimSequence* AnimSequence = GetSourceAnimAtIndex(Pose.AnimId);

			//If the animation is looping, the last Pose needs to wrap to the end
			if (AnimData->bLoop)
			{
				int32 PosesToEnd = FMath::FloorToInt(
					(AnimSequence->SequenceLength - Pose.Time) / PoseInterval);

				Pose.LastPoseId = Pose.PoseId + PosesToEnd;
			}
			else
			{
				Pose.LastPoseId = Pose.PoseId;
			}
		}

		if (AfterPose.AnimId == Pose.AnimId)
		{
			Pose.NextPoseId = AfterPose.PoseId;
		}
		else
		{
			const FMotionAnimMetaData* AnimData = GetSourceAnimMetaAtIndex(Pose.AnimId);
			const UAnimSequence* AnimSequence = GetSourceAnimAtIndex(Pose.AnimId);

			//If the animation is looping, the next Pose needs to wrap back to the beginning
			if (AnimData->bLoop)
			{
				int32 PosesToBeginning = FMath::FloorToInt(Pose.Time / PoseInterval);
				Pose.NextPoseId = Pose.PoseId - PosesToBeginning;
			}
			else
			{
				Pose.NextPoseId = Pose.PoseId;
			}
		}

		//If the Pose at the beginning of the database is looping, we need to fix its before Pose reference
		FPoseMotionData& StartPose = Poses[0];
		const FMotionAnimMetaData* StartAnimData = GetSourceAnimMetaAtIndex(StartPose.AnimId);
		const UAnimSequence* StartAnimSequence = GetSourceAnimAtIndex(StartPose.AnimId);
		if (StartAnimData->bLoop)
		{
			int32 PosesToEnd = FMath::FloorToInt((StartAnimSequence->SequenceLength - StartPose.Time) / PoseInterval);
			StartPose.LastPoseId = StartPose.PoseId + PosesToEnd;
		}

		//If the Pose at the end of the database is looping, we need to fix its after Pose reference
		FPoseMotionData& EndPose = Poses[0];
		const FMotionAnimMetaData* EndAnimData = GetSourceAnimMetaAtIndex(EndPose.AnimId);
		const UAnimSequence* EndAnimSequence = GetSourceAnimAtIndex(EndPose.AnimId);
		if (EndAnimData->bLoop)
		{
			int32 PosesToBeginning = FMath::FloorToInt(EndPose.Time / PoseInterval);
			EndPose.NextPoseId = EndPose.PoseId - PosesToBeginning;
		}
	}
}

#undef LOCTEXT_NAMESPACE