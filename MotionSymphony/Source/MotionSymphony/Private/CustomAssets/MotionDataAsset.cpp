// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "CustomAssets/MotionDataAsset.h"
#include "MotionMatchingUtil/MMPreProcessUtils.h"
#include "Data/AnimChannelState.h"
#include "Animation/AnimNotifyQueue.h"
#include "Data/AnimMirroringData.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

FMotionAnimSequence::FMotionAnimSequence()
	: bLoop(false),
	bEnableMirroring(false),
	Favour(1.0f),
	GlobalTagId(0),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	Sequence(nullptr),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr)
{
}

FMotionAnimSequence::FMotionAnimSequence(UAnimSequence* InSequence)
	: bLoop(false),
	bEnableMirroring(false),
	Favour(1.0f),
	GlobalTagId(0),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	Sequence(InSequence),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr)
{
}

UMotionAnimMetaDataWrapper::UMotionAnimMetaDataWrapper(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	bLoop(false), 
	bEnableMirroring(false),
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


void UMotionAnimMetaDataWrapper::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent)
{
	if (ParentAsset == nullptr)
		return;

	ParentAsset->MotionAnimMetaDataModified();
}

void UMotionAnimMetaDataWrapper::SetProperties(FMotionAnimSequence& MetaData)
{
	Modify();
	bLoop = MetaData.bLoop;
	bEnableMirroring = MetaData.bEnableMirroring;
	Favour = MetaData.Favour;
	bFlattenTrajectory = MetaData.bFlattenTrajectory;
	PrecedingMotion = MetaData.PrecedingMotion;
	GlobalTagId = MetaData.GlobalTagId;
	FutureTrajectory = MetaData.FutureTrajectory;
	PastTrajectory = MetaData.PastTrajectory;
	MarkPackageDirty();
}


UMotionDataAsset::UMotionDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	PoseInterval(0.1f),
	JointVelocityCalculationMethod(EJointVelocityCalculationMethod::BodyDependent),
	bOptimize(true),
	bIsProcessed(false),
	bIsOptimised(false),
//#if WITH_EDITOR	
	MotionMetaWrapper(nullptr),
	AnimMetaPreviewIndex(-1)
//#endif
{
#if WITH_EDITOR
	MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
	MotionMetaWrapper->ParentAsset = this;
#endif
}

int32 UMotionDataAsset::GetSourceAnimCount()
{
	return SourceMotionAnims.Num();
}

const FMotionAnimSequence& UMotionDataAsset::GetSourceAnimAtIndex(const int32 AnimIndex) const
{
	/*if (AnimIndex < 0 || AnimIndex > SourceMotionAnims.Num())
		return nullptr;*/

	return SourceMotionAnims[AnimIndex];
}

FMotionAnimSequence& UMotionDataAsset::GetEditableSourceAnimAtIndex(const int32 AnimIndex)
{
	return SourceMotionAnims[AnimIndex];
}

void UMotionDataAsset::AddSourceAnim(UAnimSequence* AnimSequence)
{
	if (!AnimSequence)
		return;

	//Todo:: Check if AnimSequence is valid. Maybe an dialog warning

	Modify();
	SourceMotionAnims.Emplace(FMotionAnimSequence(AnimSequence));
	MarkPackageDirty();

	bIsProcessed = false;
}

bool UMotionDataAsset::IsValidSourceAnimIndex(const int32 AnimIndex)
{
	return SourceMotionAnims.IsValidIndex(AnimIndex);
}

void UMotionDataAsset::DeleteSourceAnim(const int32 AnimIndex)
{
	if (AnimIndex < 0 || AnimIndex >= SourceMotionAnims.Num())
		return;

	Modify();
	SourceMotionAnims.RemoveAt(AnimIndex);
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
	SourceMotionAnims.Empty(SourceMotionAnims.Num() + 1);
	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	SourceMotionAnims.Empty(SourceMotionAnims.Num());
	AnimMetaPreviewIndex = -1;
#endif
}

void UMotionDataAsset::PreProcess()
{
#if WITH_EDITOR
	FScopedSlowTask MMPreProcessTask(3, LOCTEXT("Motion Matching PreProcessor", "Pre-Processing..."));
	MMPreProcessTask.MakeDialog();

	FScopedSlowTask MMPreAnimAnalyseTask(SourceMotionAnims.Num(), LOCTEXT("Motion Matching PreProcessor", "Analyzing Animation Poses"));
	MMPreAnimAnalyseTask.MakeDialog();

	if(!IsSetupValid())
		return;

	//Setup mirroring data
	ClearPoses();

	MMPreProcessTask.EnterProgressFrame();

	for (int32 i = 0; i < SourceMotionAnims.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();

		PreProcessAnim(i, false);

		if (MirroringProfile != nullptr && SourceMotionAnims[i].bEnableMirroring)
			PreProcessAnim(i, true);
	}

	GeneratePoseSequencing();

	MMPreProcessTask.EnterProgressFrame();

	if(bOptimize)
	{
		GeneratePoseCandidateTable();
		bIsOptimised = true;
	}
	else
	{
		PoseLookupTable.CandidateSets.Empty();
		bIsOptimised = false;
	}
	bIsProcessed = true;

	MMPreProcessTask.EnterProgressFrame();
#endif
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
		KMCS[i].BeginClustering(Poses, PreprocessCalibration, KMeansClusterCount, KMeansMaxIterations, false);
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
	PoseLookupTable.Process(Poses, ChosenTrajClusterSet, PreprocessCalibration, 
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

	if(SourceMotionAnims.Num() == 0)
		return false;

	if(PoseJoints.Num() == 0)
		return false;

	if(TrajectoryTimes.Num() == 0)
		return false;

	return true;
}

bool UMotionDataAsset::AreSequencesValid()
{
	for (FMotionAnimSequence& MotionAnim : SourceMotionAnims)
	{
		if (MotionAnim.Sequence == nullptr
			/*|| !MotionAnim.Sequence->IsValidToPlay()*/ //TODO: Replace with somethign that doesn't cause packaging to fail
			|| MotionAnim.Sequence->IsValidAdditive())
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
			const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState.AnimId);
			const UAnimSequence* Sequence = MotionAnim.Sequence;

			if (Sequence)
			{
				const float& CurrentSampleDataTime = ChannelState.AnimTime;
				const float CurrentTime = FMath::Clamp(ChannelState.AnimTime, 0.0f, Sequence->SequenceLength);
				const float PreviousTime = CurrentTime - DeltaTime;

				if (bGenerateNotifies)
				{
					if(NotifyTriggerMode == ENotifyTriggerMode::AllAnimations)
					{
						Sequence->GetAnimNotifies(PreviousTime, DeltaTime, MotionAnim.bLoop, Notifies);
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
					FTransform RootMotion = Sequence->ExtractRootMotion(PreviousTime, DeltaTime, MotionAnim.bLoop);

					if (ChannelState.bMirrored && MirroringProfile != nullptr)
					{
						RootMotion.Mirror(EAxis::X, EAxis::X);

						//FAnimMirroringData::MirrorTransform(RootMotion, MirroringProfile->MirrorAxis_Default);
					}

					Context.RootMotionMovementParams.AccumulateWithBlend(RootMotion, ChannelState.Weight);
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

			const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState.AnimId);
			const float PreviousTime = ChannelState.AnimTime - DeltaTime;
			bool bLooping = MotionAnim.bLoop;

			MotionAnim.Sequence->GetAnimNotifies(PreviousTime, DeltaTime, bLooping, Notifies);
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
	for (FMotionAnimSequence& MotionAnim : SourceMotionAnims)
	{
		if (MotionAnim.Sequence != nullptr)
		{
			AnimationSequences.Add(MotionAnim.Sequence);
		}
	}

	return AnimationSequences.Num() > 0;
}


void UMotionDataAsset::MotionAnimMetaDataModified()
{
	if (MotionMetaWrapper == nullptr
		|| AnimMetaPreviewIndex < 0
		|| AnimMetaPreviewIndex >= SourceMotionAnims.Num())
	{
		return;
	}

	FMotionAnimSequence& metaData = SourceMotionAnims[AnimMetaPreviewIndex];
	
	Modify();
	metaData.bLoop = MotionMetaWrapper->bLoop;
	metaData.bEnableMirroring = MotionMetaWrapper->bEnableMirroring;
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

	if (CurAnimId < 0 || CurAnimId >= SourceMotionAnims.Num())
		return false;

	MotionMetaWrapper->SetProperties(SourceMotionAnims[AnimMetaPreviewIndex]);

	return true;
}

void UMotionDataAsset::PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror /*= false*/)
{
#if WITH_EDITOR
	FMotionAnimSequence& MotionAnim = SourceMotionAnims[SourceAnimIndex];
	UAnimSequence* Sequence = MotionAnim.Sequence;

	if (!Sequence)
		return;

	Modify();

	const float AnimLength = Sequence->SequenceLength;
	float CurrentTime = 0.0f;
	float TimeHorizon = TrajectoryTimes.Last();

	if(PoseInterval < 0.01f)
		PoseInterval = 0.05f;

	while (CurrentTime <= AnimLength)
	{
		int32 PoseId = Poses.Num();
		//Todo: implement a less naive assessment of bDoNotUse checking
		bool bDoNotUse = (CurrentTime < TimeHorizon || CurrentTime > AnimLength - TimeHorizon) ? true : false;

		FVector RootVelocity;
		float RootRotVelocity;
		FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, Sequence, CurrentTime, PoseInterval);

		if (bMirror)
		{
			RootVelocity.X *= -1.0f;
			RootRotVelocity *= -1.0f;
		}

		float PoseFavour = MotionAnim.Favour; //Todo: Multiply by individual pose favour from tag tracks

		FPoseMotionData NewPoseData = FPoseMotionData(PoseId, SourceAnimIndex, CurrentTime, 
			PoseFavour, bDoNotUse, bMirror, RootRotVelocity, RootVelocity);
		
		//Process trajectory for pose
		for (int32 i = 0; i < TrajectoryTimes.Num(); ++i)
		{
			FTrajectoryPoint Point;

			if (MotionAnim.bLoop)
			{
				FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(Point, Sequence, CurrentTime, TrajectoryTimes[i]);
			}
			else
			{
				float PointTime = TrajectoryTimes[i];

				if (PointTime < 0.0f)
				{
					//past Point
					FMMPreProcessUtils::ExtractPastTrajectoryPoint(Point, Sequence, CurrentTime, PointTime,
						MotionAnim.PastTrajectory, MotionAnim.PrecedingMotion);
				}
				else
				{
					FMMPreProcessUtils::ExtractFutureTrajectoryPoint(Point, Sequence, CurrentTime, PointTime,
						MotionAnim.FutureTrajectory, MotionAnim.FollowingMotion);
				}
			}
		
			if (MotionAnim.bFlattenTrajectory)
			{
				Point.Position.Z = 0.0f;
			}

			if (bMirror)
			{
				Point.Position.X *= -1.0f;
				Point.RotationZ *= -1.0f;
			}

			NewPoseData.Trajectory.Add(Point);
		}

		const FReferenceSkeleton& RefSkeleton = Sequence->GetSkeleton()->GetReferenceSkeleton();

		//Process joints for pose
		for (int32 i = 0; i < PoseJoints.Num(); ++i)
		{
			FJointData JointData;

			if (bMirror)
			{
				FName BoneName = RefSkeleton.GetBoneName(PoseJoints[i]);
				FName MirrorBoneName = MirroringProfile->FindBoneMirror(BoneName);
				
				int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(MirrorBoneName);

				FMMPreProcessUtils::ExtractJointData(JointData, Sequence, MirrorBoneIndex, CurrentTime, PoseInterval);

				JointData.Position.X *= -1.0f;
				JointData.Velocity.X *= -1.0f;
			}
			else
			{
				FMMPreProcessUtils::ExtractJointData(JointData, Sequence, PoseJoints[i], CurrentTime, PoseInterval);
			}
			
			NewPoseData.JointData.Add(JointData);
		}

		NewPoseData.bDoNotUse = FMMPreProcessUtils::GetDoNotUseTag(Sequence, CurrentTime, PoseInterval);
		
		Poses.Add(NewPoseData);
		CurrentTime += PoseInterval;
	}
#endif
}

void UMotionDataAsset::GeneratePoseSequencing()
{
	for (int32 i = 0; i < Poses.Num(); ++i)
	{
		FPoseMotionData& Pose = Poses[i];
		FPoseMotionData& BeforePose = Poses[FMath::Max(0, i - 1)];
		FPoseMotionData& AfterPose = Poses[FMath::Min(i + 1, Poses.Num() - 1)];

		if (BeforePose.AnimId == Pose.AnimId && BeforePose.bMirrored == AfterPose.bMirrored)
		{
			Pose.LastPoseId = BeforePose.PoseId;
		}
		else
		{
			const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(Pose.AnimId);

			//If the animation is looping, the last Pose needs to wrap to the end
			if (MotionAnim.bLoop)
			{
				int32 PosesToEnd = FMath::FloorToInt(
					(MotionAnim.Sequence->SequenceLength - Pose.Time) / PoseInterval);

				Pose.LastPoseId = Pose.PoseId + PosesToEnd;
			}
			else
			{
				Pose.LastPoseId = Pose.PoseId;
			}
		}

		if (AfterPose.AnimId == Pose.AnimId && AfterPose.bMirrored == Pose.bMirrored)
		{
			Pose.NextPoseId = AfterPose.PoseId;
		}
		else
		{
			const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(Pose.AnimId);

			//If the animation is looping, the next Pose needs to wrap back to the beginning
			if (MotionAnim.bLoop)
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
		const FMotionAnimSequence& StartMotionAnim = GetSourceAnimAtIndex(StartPose.AnimId);

		if (StartMotionAnim.bLoop)
		{
			int32 PosesToEnd = FMath::FloorToInt((StartMotionAnim.Sequence->SequenceLength - StartPose.Time) / PoseInterval);
			StartPose.LastPoseId = StartPose.PoseId + PosesToEnd;
		}

		//If the Pose at the end of the database is looping, we need to fix its after Pose reference
		FPoseMotionData& EndPose = Poses[0];
		const FMotionAnimSequence& EndMotionAnim = GetSourceAnimAtIndex(EndPose.AnimId);

		if (EndMotionAnim.bLoop)
		{
			int32 PosesToBeginning = FMath::FloorToInt(EndPose.Time / PoseInterval);
			EndPose.NextPoseId = EndPose.PoseId - PosesToBeginning;
		}
	}
}

#undef LOCTEXT_NAMESPACE