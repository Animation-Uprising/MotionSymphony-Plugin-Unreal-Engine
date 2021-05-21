// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "Data/MotionAnimAsset.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimComposite.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"

#define LOCTEXT_NAMESPACE "MotionAnimAsset"

FMotionAnimAsset::FMotionAnimAsset()
	: AnimId(0),
	MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(nullptr),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr),
	CostMultiplier(1.0f),
	ParentMotionDataAsset(nullptr)
{
}

FMotionAnimAsset::FMotionAnimAsset(UAnimationAsset* InAnimAsset, UMotionDataAsset* InParentMotionDataAsset)
	: AnimId(0),
	MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),	
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(InAnimAsset),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr),
	CostMultiplier(1.0f),
	ParentMotionDataAsset(InParentMotionDataAsset)
{
}

FMotionAnimAsset::~FMotionAnimAsset()
{
}

double FMotionAnimAsset::GetAnimLength() const
{
	return 0.0;
}

double FMotionAnimAsset::GetFrameRate() const
{
	return 30.0;
}

int32 FMotionAnimAsset::GetTickResolution() const
{
	return FMath::RoundToInt(/*(double)GetDefault<UPersonaOptions>()->TimelineScrubSnapValue **/ GetFrameRate());
}

void FMotionAnimAsset::PostLoad()
{
#if WITH_EDITOR
	InitializeTagTrack();
#endif
	RefreshCacheData();
}

void FMotionAnimAsset::SortTags()
{
	Tags.Sort();
}

bool FMotionAnimAsset::RemoveTags(const TArray<FName>& TagsToRemove)
{
	bool bSequenceModifiers = false;
	for (int32 TagIndex = Tags.Num() - 1; TagIndex >= 0; --TagIndex)
	{
		FAnimNotifyEvent& AnimTags = Tags[TagIndex];
		if (TagsToRemove.Contains(AnimTags.NotifyName))
		{
			if (!bSequenceModifiers)
			{
				//Modify(); //This is a struct within a package, can't mark as modified here
				bSequenceModifiers = true;
			}

			Tags.RemoveAtSwap(TagIndex);
		}
	}

	if (bSequenceModifiers)
	{
		//MarkPackageDirty(); //This is a struct within a package. Can't mark it as dirty here
		RefreshCacheData();
	}

	return bSequenceModifiers;
}

void FMotionAnimAsset::GetMotionTags(const float& StartTime, const float& DeltaTime, const bool bAllowLooping, TArray<FAnimNotifyEventReference>& OutActiveNotifies) const
{
	if (DeltaTime == 0.f)
	{
		return;
	}

	// Early out if we have no notifies
	if (!IsTagAvailable())
	{
		return;
	}

	bool const bPlayingBackwards = false;
	float PreviousPosition = StartTime;
	float CurrentPosition = StartTime;
	float DesiredDeltaMove = DeltaTime;

	float AnimLength = GetAnimLength();

	do
	{
		// Disable looping here. Advance to desired position, or beginning / end of animation 
		const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, CurrentPosition, AnimLength);

		// Verify position assumptions
	/*	ensureMsgf(bPlayingBackwards ? (CurrentPosition <= PreviousPosition) : (CurrentPosition >= PreviousPosition), TEXT("in Animation %s(Skeleton %s) : bPlayingBackwards(%d), PreviousPosition(%0.2f), Current Position(%0.2f)"),
			*GetName(), *GetNameSafe(GetSkeleton()), bPlayingBackwards, PreviousPosition, CurrentPosition);*/

		GetMotionTagsFromDeltaPositions(PreviousPosition, CurrentPosition, OutActiveNotifies);

		// If we've hit the end of the animation, and we're allowed to loop, keep going.
		if ((AdvanceType == ETAA_Finished) && bAllowLooping)
		{
			const float ActualDeltaMove = (CurrentPosition - PreviousPosition);
			DesiredDeltaMove -= ActualDeltaMove;

			PreviousPosition = bPlayingBackwards ? AnimLength : 0.f;
			CurrentPosition = PreviousPosition;
		}
		else
		{
			break;
		}
	} while (true);
}

void FMotionAnimAsset::GetMotionTagsFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition, TArray<FAnimNotifyEventReference>& OutActiveTags) const
{
	// Early out if we have no notifies
	if ((Tags.Num() == 0) || (PreviousPosition > CurrentPosition))
	{
		return;
	}

	bool const bPlayingBackwards = false;

	for (int32 TagIndex = 0; TagIndex < Tags.Num(); ++TagIndex)
	{
		const FAnimNotifyEvent& AnimNotifyEvent = Tags[TagIndex];
		const float TagStartTime = AnimNotifyEvent.GetTriggerTime();
		const float TagEndTime = AnimNotifyEvent.GetEndTriggerTime();

		if ((TagStartTime <= CurrentPosition) && (TagEndTime > PreviousPosition))
		{
			//OutActiveTags.Emplace(&AnimNotifyEvent, this);
			OutActiveTags.Emplace(&AnimNotifyEvent, AnimAsset);
		}
	}
}

void FMotionAnimAsset::GetRootBoneTransform(FTransform& OutTransform, const float Time) const
{
	OutTransform = FTransform::Identity;
}

void FMotionAnimAsset::CacheTrajectoryPoints(TArray<FVector>& OutTrajectoryPoints) const
{
	OutTrajectoryPoints.Empty();
}

void FMotionAnimAsset::InitializeTagTrack()
{
#if WITH_EDITORONLY_DATA
	if (MotionTagTracks.Num() == 0)
	{
		MotionTagTracks.Add(FAnimNotifyTrack(TEXT("1"), FLinearColor::White));
	}
#endif
}

void FMotionAnimAsset::ClampTagAtEndOfSequence()
{
	const float AnimLength = GetAnimLength();
	const float TagClampTime = AnimLength - 0.01f; //Slight offset so that notify is still draggable
	for (int i = 0; i < Tags.Num(); ++i)
	{
		if (Tags[i].GetTime() >= AnimLength)
		{
			Tags[i].SetTime(TagClampTime);
			Tags[i].TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
		}
	}
}

//uint8* FMotionAnimAsset::FindTagPropertyData(int32 TagIndex, FArrayProperty*& ArrayProperty)
//{
//	ArrayProperty = nullptr;
//
//	if (Tags.IsValidIndex(TagIndex))
//	{
//		return FindArrayProperty(TEXT("Tags"), ArrayProperty, TagIndex);
//	}
//
//	return nullptr;
//}

bool CanNotifyUseTrack(const FAnimNotifyTrack& Track, const FAnimNotifyEvent& Notify)
{
	for (const FAnimNotifyEvent* Event : Track.Notifies)
	{
		if (FMath::IsNearlyEqual(Event->GetTime(), Notify.GetTime()))
		{
			return false;
		}
	}
	return true;
}

FAnimNotifyTrack& AddNewTrack(TArray<FAnimNotifyTrack>& Tracks)
{
	const int32 Index = Tracks.Add(FAnimNotifyTrack(*FString::FromInt(Tracks.Num() + 1), FLinearColor::White));
	return Tracks[Index];
}

void FMotionAnimAsset::RefreshCacheData()
{
	SortTags();

#if WITH_EDITORONLY_DATA
	for (int32 TrackIndex = 0; TrackIndex < MotionTagTracks.Num(); ++TrackIndex)
	{
		MotionTagTracks[TrackIndex].Notifies.Empty();
	}

	for (FAnimNotifyEvent& Tag : Tags)
	{
		if (!MotionTagTracks.IsValidIndex(Tag.TrackIndex))
		{
			// This really shouldn't happen (unless we are a cooked asset), but try to handle it
			//ensureMsgf(GetOutermost()->bIsCookedForEditor, TEXT("AnimNotifyTrack: Anim (%s) has notify (%s) with track index (%i) that does not exist"), *GetFullName(), *Notify.NotifyName.ToString(), Notify.TrackIndex);

			if (Tag.TrackIndex < 0 || Tag.TrackIndex > 20)
			{
				Tag.TrackIndex = 0;
			}

			while (!MotionTagTracks.IsValidIndex(Tag.TrackIndex))
			{
				AddNewTrack(MotionTagTracks);
			}
		}

		//Handle overlapping tags
		FAnimNotifyTrack* TrackToUse = nullptr;
		int32 TrackIndexToUse = INDEX_NONE;
		for (int32 TrackOffset = 0; TrackOffset < MotionTagTracks.Num(); ++TrackOffset)
		{
			const int32 TrackIndex = (Tag.TrackIndex + TrackOffset) % MotionTagTracks.Num();
			if (CanNotifyUseTrack(MotionTagTracks[TrackIndex], Tag))
			{
				TrackToUse = &MotionTagTracks[TrackIndex];
				TrackIndexToUse = TrackIndex;
				break;
			}
		}

		if (TrackToUse == nullptr)
		{
			TrackToUse = &AddNewTrack(MotionTagTracks);
			TrackIndexToUse = MotionTagTracks.Num() - 1;
		}

		check(TrackToUse);
		check(TrackIndexToUse != INDEX_NONE);

		Tag.TrackIndex = TrackIndexToUse;
		TrackToUse->Notifies.Add(&Tag);
	}

	// this is a separate loop of checking if they contains valid notifies
	for (int32 TagIndex = 0; TagIndex < Tags.Num(); ++TagIndex)
	{
		const FAnimNotifyEvent& Tag = Tags[TagIndex];
		//make sure if they can be placed in editor
		if (Tag.Notify)
		{
			//if (Tag.Notify->CanBePlaced(this) == false)
			//{
			//	static FName NAME_LoadErrors("LoadErrors");
			//	FMessageLog LoadErrors(NAME_LoadErrors);

			//	TSharedRef<FTokenizedMessage> Message = LoadErrors.Error();
			//	Message->AddToken(FTextToken::Create(LOCTEXT("InvalidMotionTag1", "The Animation ")));
			//	//Message->AddToken(FAssetNameToken::Create(GetPathName(), FText::FromString(GetNameSafe(this))));
			//	Message->AddToken(FTextToken::Create(LOCTEXT("InvalidMotionTag2", " contains invalid tag ")));
			//	Message->AddToken(FAssetNameToken::Create(Tag.Notify->GetPathName(), FText::FromString(GetNameSafe(Tag.Notify))));
			//	LoadErrors.Open();
			//}
		}

		if (Tag.NotifyStateClass)
		{
			//if (Tag.NotifyStateClass->CanBePlaced(this) == false)
			//{
			//	static FName NAME_LoadErrors("LoadErrors");
			//	FMessageLog LoadErrors(NAME_LoadErrors);

			//	TSharedRef<FTokenizedMessage> Message = LoadErrors.Error();
			//	Message->AddToken(FTextToken::Create(LOCTEXT("InvalidMotionTag1", "The Animation ")));
			//	//Message->AddToken(FAssetNameToken::Create(GetPathName(), FText::FromString(GetNameSafe(this))));
			//	Message->AddToken(FTextToken::Create(LOCTEXT("InvalidMotionTag2", " contains invalid Tag ")));
			//	Message->AddToken(FAssetNameToken::Create(Tag.NotifyStateClass->GetPathName(), FText::FromString(GetNameSafe(Tag.NotifyStateClass))));
			//	LoadErrors.Open();
			//}
		}
	}
	//tag broadcast
	OnTagChanged.Broadcast();
#endif //WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
void FMotionAnimAsset::RegisterOnTagChanged(const FOnTagChanged& Delegate)
{
	OnTagChanged.Add(Delegate);
}

void FMotionAnimAsset::UnRegisterOnTagChanged(void* Unregister)
{
	if (!Unregister)
	{
		return;
	}

	OnTagChanged.RemoveAll(Unregister);
}
#endif

bool FMotionAnimAsset::IsTagAvailable() const
{
	return (Tags.Num() != 0) && (GetAnimLength() > 0.0f);
}

FMotionAnimSequence::FMotionAnimSequence()
	: FMotionAnimAsset(),
	Sequence(nullptr)
{
	MotionAnimAssetType = EMotionAnimAssetType::Sequence;
}

FMotionAnimSequence::FMotionAnimSequence(UAnimSequence* InSequence, UMotionDataAsset* InParentMotionData)
	: FMotionAnimAsset(InSequence, InParentMotionData),
	Sequence(InSequence)
{
	MotionAnimAssetType = EMotionAnimAssetType::Sequence;
}


FMotionAnimSequence::~FMotionAnimSequence()
{
}

double FMotionAnimSequence::GetAnimLength() const
{
	return Sequence ? Sequence->SequenceLength : 0.0f;
}

double FMotionAnimSequence::GetFrameRate() const
{
	return Sequence ? Sequence->GetFrameRate() : 30.0;
}

void FMotionAnimSequence::GetRootBoneTransform(FTransform& OutTransform, const float Time) const
{
	if (!Sequence)
	{
		OutTransform = FTransform::Identity;
		return;
	}

	Sequence->GetBoneTransform(OutTransform, 0, Time, false);
}

void FMotionAnimSequence::CacheTrajectoryPoints(TArray<FVector>& OutTrajectoryPoints) const
{
	for (float time = 0.1f; time < Sequence->SequenceLength; time += 0.1f)
	{
		OutTrajectoryPoints.Add(Sequence->ExtractRootMotion(0.0f, time, false).GetLocation());
	}
}

FMotionBlendSpace::FMotionBlendSpace()
	: FMotionAnimAsset(),
	BlendSpace(nullptr),
	SampleSpacing(0.1f, 0.1f)
{
	MotionAnimAssetType = EMotionAnimAssetType::BlendSpace;
}

FMotionBlendSpace::FMotionBlendSpace(UBlendSpaceBase* InBlendSpace, UMotionDataAsset* InParentMotionData)
	: FMotionAnimAsset(InBlendSpace, InParentMotionData),
	BlendSpace(InBlendSpace),
	SampleSpacing(0.1f, 0.1f)
{
	MotionAnimAssetType = EMotionAnimAssetType::BlendSpace;
}

FMotionBlendSpace::~FMotionBlendSpace()
{
}

double FMotionBlendSpace::GetAnimLength() const
{
	return BlendSpace ? BlendSpace->AnimLength : 0.0f;
}

double FMotionBlendSpace::GetFrameRate() const
{
	return BlendSpace ? 30.0 : 30.0f; //Todo: Do this properly for blend spaces
}

FMotionComposite::FMotionComposite()
	: FMotionAnimAsset(),
	 AnimComposite(nullptr)
{
	MotionAnimAssetType = EMotionAnimAssetType::Composite;
}

FMotionComposite::FMotionComposite(class UAnimComposite* InComposite, UMotionDataAsset* InParentMotionData)
	: FMotionAnimAsset(InComposite, InParentMotionData),
	AnimComposite(InComposite)
{
	MotionAnimAssetType = EMotionAnimAssetType::Composite;
}

FMotionComposite::~FMotionComposite()
{
}

double FMotionComposite::GetAnimLength() const
{
	return AnimComposite ? AnimComposite->SequenceLength : 0.0;
}

double FMotionComposite::GetFrameRate() const
{
	return AnimComposite ? 30.0 : 30.0;
}

void FMotionComposite::GetRootBoneTransform(FTransform& OutTransform, const float Time) const
{
	if (!AnimComposite)
	{
		OutTransform = FTransform::Identity;
		return;
	}

	float RemainingTime = Time;
	for (int32 i = 0; i < AnimComposite->AnimationTrack.AnimSegments.Num(); ++i)
	{
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimComposite->AnimationTrack.AnimSegments[i].AnimReference);
		
		if (!AnimSequence)
		{
			break;
		}

		FTransform LocalBoneTransform = FTransform::Identity;
		if (AnimSequence->SequenceLength >= RemainingTime)
		{
			AnimSequence->GetBoneTransform(LocalBoneTransform, 0, RemainingTime, false);
			OutTransform = LocalBoneTransform * OutTransform;
			break;
		}
		else
		{
			AnimSequence->GetBoneTransform(LocalBoneTransform, 0, AnimSequence->SequenceLength, false);
			OutTransform = LocalBoneTransform * OutTransform;
			RemainingTime -= AnimSequence->SequenceLength;
		}
	}
}

void FMotionComposite::CacheTrajectoryPoints(TArray<FVector>& OutTrajectoryPoints) const
{
	FTransform CumulativeTransform = FTransform::Identity;
	for (int32 i = 0; i < AnimComposite->AnimationTrack.AnimSegments.Num(); ++i)
	{
		UAnimSequence* AnimSequence = Cast<UAnimSequence>(AnimComposite->AnimationTrack.AnimSegments[i].AnimReference);

		FTransform LocalRootMotionTransform = FTransform::Identity;
		for (float Time = 0.1f; Time <= AnimSequence->SequenceLength; Time += 0.1f)
		{
			LocalRootMotionTransform = AnimSequence->ExtractRootMotion(0.0f, Time, false);
			LocalRootMotionTransform = LocalRootMotionTransform * CumulativeTransform;

			OutTrajectoryPoints.Add(LocalRootMotionTransform.GetLocation());
		}

		CumulativeTransform = AnimSequence->ExtractRootMotion(0.0f, AnimSequence->SequenceLength, false) * CumulativeTransform;
	}
}

#undef LOCTEXT_NAMESPACE