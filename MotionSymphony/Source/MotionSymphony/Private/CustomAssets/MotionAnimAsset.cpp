
#include "CustomAssets/MotionAnimAsset.h"
#include "Preferences/PersonaOptions.h"

#define LOCTEXT_NAMESPACE "MotionAnimAsset"

FMotionAnimAsset::FMotionAnimAsset()
	: MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(nullptr),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr),
	DistanceMatchType(EDistanceMatchType::None),
	DistanceMatchBasis(EDistanceMatchBasis::Positional),
	CostMultiplier(1.0f)
{
}

FMotionAnimAsset::FMotionAnimAsset(UAnimationAsset* InAnimAsset)
	: MotionAnimAssetType(EMotionAnimAssetType::None),
	bLoop(false),
	bEnableMirroring(false),	
	bFlattenTrajectory(true),
	PastTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	FutureTrajectory(ETrajectoryPreProcessMethod::IgnoreEdges),
	AnimAsset(InAnimAsset),
	PrecedingMotion(nullptr),
	FollowingMotion(nullptr),
	DistanceMatchType(EDistanceMatchType::None),
	DistanceMatchBasis(EDistanceMatchBasis::Positional),
	CostMultiplier(1.0f)
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

void FMotionAnimAsset::InitializeTagTrack()
{
	if (MotionTagTracks.Num() == 0)
	{
		MotionTagTracks.Add(FAnimNotifyTrack(TEXT("1"), FLinearColor::White));
	}
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

#if WITH_EDITOR
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
#endif //WITH_EDITOR
}

void FMotionAnimAsset::RegisterOnTagChanged(const FOnTagChanged& Delegate)
{
	OnTagChanged.Add(Delegate);
}

void FMotionAnimAsset::UnRegisterOnTagChanged(void* Unregister)
{
	OnTagChanged.RemoveAll(Unregister);
}

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

FMotionAnimSequence::FMotionAnimSequence(UAnimSequence* InSequence)
	: FMotionAnimAsset(InSequence),
	Sequence(InSequence)
{
	MotionAnimAssetType = EMotionAnimAssetType::Sequence;
}


FMotionAnimSequence::~FMotionAnimSequence()
{

}

double FMotionAnimSequence::GetAnimLength() const
{
	return Sequence == nullptr ? 0.0f : Sequence->SequenceLength;
}

double FMotionAnimSequence::GetFrameRate() const
{
	return Sequence == nullptr ? 30.0 : Sequence->GetFrameRate();
}

FMotionBlendSpace::FMotionBlendSpace()
	: FMotionAnimAsset(),
	BlendSpace(nullptr),
	SampleSpacing(0.1f, 0.1f)
{
	MotionAnimAssetType = EMotionAnimAssetType::BlendSpace;
}

FMotionBlendSpace::FMotionBlendSpace(UBlendSpaceBase* InBlendSpace)
	: FMotionAnimAsset(InBlendSpace),
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
	return BlendSpace == nullptr ? 0.0f : BlendSpace->AnimLength;
}

double FMotionBlendSpace::GetFrameRate() const
{
	return BlendSpace == nullptr ? 30.0 : 30.0f; //Todo: Do this properly for blend spaces
}

#undef LOCTEXT_NAMESPACE