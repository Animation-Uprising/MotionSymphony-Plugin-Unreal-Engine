// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "CustomAssets/MotionDataAsset.h"

#include "MotionMatchingUtils.h"
#include "Data/MotionAnimMetaDataWrapper.h"
#include "MotionMatchingUtil/MMPreProcessUtils.h"
#include "Data/AnimChannelState.h"
#include "Animation/AnimNotifyQueue.h"
#include "Data/AnimMirroringData.h"
#include "Misc/ScopedSlowTask.h"
#include "Animation/BlendSpace.h"
#include "Tags/TagSection.h"
#include "Tags/TagPoint.h"
#include "MotionMatchingUtil/MMBlueprintFunctionLibrary.h"

#if WITH_EDITOR
#include "AnimationEditorUtils.h"
#include "Misc/MessageDialog.h"
#endif


#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

FDistanceMatchIdentifier::FDistanceMatchIdentifier()
	: MatchType(EDistanceMatchType::None),
	MatchBasis(EDistanceMatchBasis::Positional)
{
}

FDistanceMatchIdentifier::FDistanceMatchIdentifier(EDistanceMatchType InMatchType, EDistanceMatchBasis InMatchBasis)
	: MatchType(InMatchType),
	MatchBasis(InMatchBasis)
{
}

FDistanceMatchIdentifier::FDistanceMatchIdentifier(FDistanceMatchSection& InDistanceMatchSection)
	: MatchType(InDistanceMatchSection.MatchType),
	MatchBasis(InDistanceMatchSection.MatchBasis)
{
}

FDistanceMatchIdentifier::FDistanceMatchIdentifier(const FDistanceMatchSection& InDistanceMatchSection)
	: MatchType(InDistanceMatchSection.MatchType),
	MatchBasis(InDistanceMatchSection.MatchBasis)
{
}

bool FDistanceMatchIdentifier::operator==(const FDistanceMatchIdentifier rhs) const
{
	return (MatchType == rhs.MatchType) && (MatchBasis == rhs.MatchBasis);
}

UMotionDataAsset::UMotionDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	PoseInterval(0.1f),
	MotionMatchConfig(nullptr),
	JointVelocityCalculationMethod(EJointVelocityCalculationMethod::BodyDependent),
	NotifyTriggerMode(ENotifyTriggerMode::HighestWeightedAnimation),
	bOptimize(true),
	OptimisationModule(nullptr),
	PreprocessCalibration(nullptr),
	MirroringProfile(nullptr),
	bIsProcessed(false),
	bIsOptimised(false),
	MotionMetaWrapper(nullptr),
	AnimMetaPreviewIndex(-1),
	AnimMetaPreviewType(EMotionAnimAssetType::None)
{
#if WITH_EDITOR
	MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
	MotionMetaWrapper->ParentAsset = this;
#endif

}

int32 UMotionDataAsset::GetAnimCount() const
{
	return SourceMotionAnims.Num() + SourceBlendSpaces.Num() + SourceComposites.Num();
}

int32 UMotionDataAsset::GetSourceAnimCount() const
{
	return SourceMotionAnims.Num();
}

int32 UMotionDataAsset::GetSourceBlendSpaceCount() const
{
	return SourceBlendSpaces.Num();
}

int32 UMotionDataAsset::GetSourceCompositeCount() const
{
	return SourceComposites.Num();
}

FMotionAnimAsset* UMotionDataAsset::GetSourceAnim(const int32 AnimId, const EMotionAnimAssetType AnimType)
{
	switch (AnimType)
	{
	case EMotionAnimAssetType::Sequence:
	{
		if (AnimId < 0 || AnimId >= SourceMotionAnims.Num())
		{
			return nullptr;
		}

		return &SourceMotionAnims[AnimId];
	}
	case EMotionAnimAssetType::BlendSpace:
	{
		if (AnimId < 0 || AnimId >= SourceBlendSpaces.Num())
		{
			return nullptr;
		}

		return &SourceBlendSpaces[AnimId];
	}
	case EMotionAnimAssetType::Composite:
	{
		if (AnimId < 0 || AnimId >= SourceComposites.Num())
		{
			return nullptr;
		}

		return &SourceComposites[AnimId];
	}

	default: return nullptr;
	}
}

const FMotionAnimSequence& UMotionDataAsset::GetSourceAnimAtIndex(const int32 AnimIndex) const
{
	return SourceMotionAnims[AnimIndex];
}

const FMotionBlendSpace& UMotionDataAsset::GetSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex) const
{
	return SourceBlendSpaces[BlendSpaceIndex];
}

const FMotionComposite& UMotionDataAsset::GetSourceCompositeAtIndex(const int32 CompositeIndex) const
{
	return SourceComposites[CompositeIndex];
}

FMotionAnimSequence& UMotionDataAsset::GetEditableSourceAnimAtIndex(const int32 AnimIndex)
{
	return SourceMotionAnims[AnimIndex];
}

FMotionBlendSpace& UMotionDataAsset::GetEditableSourceBlendSpaceAtIndex(const int32 BlendSpaceIndex)
{
	return SourceBlendSpaces[BlendSpaceIndex];
}


FMotionComposite& UMotionDataAsset::GetEditableSourceCompositeAtIndex(const int32 CompositeIndex)
{
	return SourceComposites[CompositeIndex];
}

void UMotionDataAsset::AddSourceAnim(UAnimSequence* AnimSequence)
{
	if (!AnimSequence)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to add source animation, the source animation is null"))
		return;
	}

	Modify();
	SourceMotionAnims.Emplace(FMotionAnimSequence(AnimSequence, this));
	SourceMotionAnims.Last().AnimId = SourceMotionAnims.Num() - 1;
	MarkPackageDirty();

	bIsProcessed = false;
}

void UMotionDataAsset::AddSourceBlendSpace(UBlendSpace* BlendSpace)
{
	if(!BlendSpace)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to add source blend space, the source blend sapce is null"))
		return;
	}

	Modify();
	SourceBlendSpaces.Emplace(FMotionBlendSpace(BlendSpace, this));
	SourceBlendSpaces.Last().AnimId = SourceBlendSpaces.Num() - 1;
	MarkPackageDirty();

	bIsProcessed = false;
}

void UMotionDataAsset::AddSourceComposite(UAnimComposite* Composite)
{
	if(!Composite)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to add source composite, the source composite is null"))
		return;
	}

	Modify();
	SourceComposites.Emplace(FMotionComposite(Composite, this));
	SourceComposites.Last().AnimId = SourceComposites.Num() - 1;
	MarkPackageDirty();

	bIsProcessed = false;
}

bool UMotionDataAsset::IsValidSourceAnimIndex(const int32 AnimIndex)
{
	return SourceMotionAnims.IsValidIndex(AnimIndex);
}

bool UMotionDataAsset::IsValidSourceBlendSpaceIndex(const int32 BlendSpaceIndex)
{
	return SourceBlendSpaces.IsValidIndex(BlendSpaceIndex);
}

bool UMotionDataAsset::IsValidSourceCompositeIndex(const int32 CompositeIndex)
{
	return SourceComposites.IsValidIndex(CompositeIndex);
}

void UMotionDataAsset::DeleteSourceAnim(const int32 AnimIndex)
{
	if (AnimIndex < 0 || AnimIndex >= SourceMotionAnims.Num())
	{ 
		UE_LOG(LogTemp, Error, TEXT("Failed to delete source animation. The anim index is out of range."))
		return;
	}

	Modify();
	SourceMotionAnims.RemoveAt(AnimIndex);

	for(int i = AnimIndex; i < SourceMotionAnims.Num(); ++i)
	{
		SourceMotionAnims[i].AnimId -= 1;
	}
	
	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	if (AnimIndex < AnimMetaPreviewIndex)
	{
		SetAnimMetaPreviewIndex(EMotionAnimAssetType::Sequence, AnimMetaPreviewIndex - 1);
	}
#endif
}

void UMotionDataAsset::DeleteSourceBlendSpace(const int32 BlendSpaceIndex)
{
	if(BlendSpaceIndex < 0 || BlendSpaceIndex >= SourceBlendSpaces.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to delete source blend space. The blend space index is out of range."))
		return;
	}

	Modify();
	SourceBlendSpaces.RemoveAt(BlendSpaceIndex);

	for(int i = BlendSpaceIndex; i < SourceBlendSpaces.Num(); ++i)
	{
		SourceBlendSpaces[i].AnimId -= 1;
	}

	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	if (BlendSpaceIndex < AnimMetaPreviewIndex)
	{
		SetAnimMetaPreviewIndex(EMotionAnimAssetType::BlendSpace, AnimMetaPreviewIndex - 1);
	}
#endif
}

void UMotionDataAsset::DeleteSourceComposite(const int32 CompositeIndex)
{
	if(CompositeIndex < 0 || CompositeIndex >= SourceComposites.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to add source composite. The composite index is out of range."))
		return;
	}

	Modify();
	SourceComposites.RemoveAt(CompositeIndex);

	for(int i = CompositeIndex; i < SourceComposites.Num(); ++i)
	{
		SourceComposites[i].AnimId -= 1;
	}
	
	bIsProcessed = false;
	MarkPackageDirty();

#if WITH_EDITOR
	if (CompositeIndex < AnimMetaPreviewIndex)
	{
		SetAnimMetaPreviewIndex(EMotionAnimAssetType::Composite, AnimMetaPreviewIndex - 1);
	}
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


void UMotionDataAsset::ClearSourceBlendSpaces()
{
	Modify();
	SourceBlendSpaces.Empty(SourceBlendSpaces.Num() + 1);
	bIsProcessed = false;
	MarkPackageDirty();
}

void UMotionDataAsset::ClearSourceComposites()
{
	Modify();
	SourceComposites.Empty(SourceComposites.Num() + 1);
	bIsProcessed = false;
	MarkPackageDirty();
}

bool UMotionDataAsset::CheckValidForPreProcess() const
{
	bool bValid = true;

	//Check that the motion matching config is set
	if (!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData PreProcess Validity Check Failed: Missing MotionMatchConfig reference."));
		return false;
	}

	//Check that there is a Skeleton set
	if (!MotionMatchConfig->GetSourceSkeleton())
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: Skeleton not set on MotionMatchConfig asset"));
		bValid = false;
	}

	//Check that there are pose joints set to match
	if (MotionMatchConfig->PoseBones.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No pose bones set. Check your MotionMatchConfig asset"));
		bValid = false;
	}

	//Check that there are trajectory points set
	if (MotionMatchConfig->TrajectoryTimes.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No trajectory times set"));
		bValid = false;
	}

	//Check that there is at least one animation to pre-process
	const int32 SourceAnimCount = GetSourceAnimCount() + GetSourceBlendSpaceCount() + GetSourceCompositeCount();
	if (SourceAnimCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: No animations added"));
		bValid = false;
	}

	//Check that there is at least one trajectory point in the future
	float HighestTrajPoint = -1.0f;
	for (int32 i = 0; i < MotionMatchConfig->TrajectoryTimes.Num(); ++i)
	{
		const float TimeValue = MotionMatchConfig->TrajectoryTimes[i];

		if (TimeValue > 0.0f)
		{
			HighestTrajPoint = TimeValue;
			break;
		}
	}

	if (HighestTrajPoint <= 0.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data PreProcess Validity Check Failed: Must have at least one trajectory point set in the future"));
		bValid = false;
	}

	if (!bValid)
	{
#if WITH_EDITOR
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Invalid Motion Data",
			"The current setup of the motion data asset is not valid for pre-processing. Please see the output log for more details."));
#endif
	}

	return bValid;
}

void UMotionDataAsset::PreProcess()
{
#if WITH_EDITOR
	if (!IsSetupValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Failed to PreProcess",
			"The Motion Data asset failed to pre-process the animation database due to invalid setup. Please fix all errors in the error log and try pre-processing again."));
		return;
	}

	FScopedSlowTask MMPreProcessTask(3, LOCTEXT("Motion Matching PreProcessor", "Pre-Processing..."));
	MMPreProcessTask.MakeDialog();

	FScopedSlowTask MMPreAnimAnalyseTask(SourceMotionAnims.Num() + SourceBlendSpaces.Num() + SourceComposites.Num(), LOCTEXT("Motion Matching PreProcessor", "Analyzing Animation Poses"));
	MMPreAnimAnalyseTask.MakeDialog();

	MotionMatchConfig->Initialize();

	//Setup mirroring data
	ClearPoses();
	
	MMPreProcessTask.EnterProgressFrame();

	//Animation Sequences
	for (int32 i = 0; i < SourceMotionAnims.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();

		PreProcessAnim(i, false);

		if (MirroringProfile != nullptr && SourceMotionAnims[i].bEnableMirroring)
		{
			PreProcessAnim(i, true);
		}
	}

	//Blend Spaces
	for (int32 i = 0; i < SourceBlendSpaces.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();

		PreProcessBlendSpace(i, false);

		if(MirroringProfile != nullptr && SourceBlendSpaces[i].bEnableMirroring)
		{
			PreProcessBlendSpace(i, true);
		}
	}

	//Composites
	for (int32 i = 0; i < SourceComposites.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();
		
		PreProcessComposite(i, false);

		if (MirroringProfile != nullptr && SourceComposites[i].bEnableMirroring)
		{
			PreProcessComposite(i, true);
		}
	}

	//Fix Joints for root Rotation
	

	GeneratePoseSequencing();

	MMPreProcessTask.EnterProgressFrame();

	//Standard deviations
	//First Find a list of traits
	TArray<FMotionTraitField> UsedMotionTraits;
	for (int32 i = 0; i < Poses.Num(); ++i)
	{
		UsedMotionTraits.AddUnique(Poses[i].Traits);
	}

	FeatureStandardDeviations.Empty(UsedMotionTraits.Num());

	for (const FMotionTraitField& MotionTrait : UsedMotionTraits)
	{
		FCalibrationData& NewCalibrationData = FeatureStandardDeviations.Add(MotionTrait, FCalibrationData(this));
		NewCalibrationData.GenerateStandardDeviationWeights(this, MotionTrait);
	}

	PreprocessCalibration->Initialize();

	if(bOptimize && OptimisationModule)
	{
		OptimisationModule->BuildOptimisationStructures(this);
		bIsOptimised = true;
	}
	else
	{
		bIsOptimised = false;
	}

	bIsProcessed = true;

	MMPreProcessTask.EnterProgressFrame();
#endif
}



void UMotionDataAsset::ClearPoses()
{
	Poses.Empty();
	DistanceMatchSections.Empty();
	bIsProcessed = false;
}

bool UMotionDataAsset::IsSetupValid()
{
	bool bValidSetup = true;

	if(!MotionMatchConfig || !MotionMatchConfig->IsSetupValid())
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData setup is invalid. MotionMatchConfig property is not setup correctly."));
		bValidSetup = false;
	}
	else
	{
		SetSkeleton(MotionMatchConfig->GetSourceSkeleton());

		//Check mirroring profile is valid
		if (MirroringProfile)
		{
			if (!MirroringProfile->IsSetupValid()
				|| MirroringProfile->GetSourceSkeleton() != MotionMatchConfig->SourceSkeleton)
			{
				UE_LOG(LogTemp, Error, TEXT("Motion Data setup is invalid. The Mirroring Profile is either invalid or not compatible with the motion match config (i.e. they don't use the same skeleton)"));
				bValidSetup = false;
			}
		}
	}

	if (!PreprocessCalibration || !PreprocessCalibration->IsSetupValid(MotionMatchConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData setup is invalid. PreprocessCalibration property is not setup correctly."));
		bValidSetup = false;
	}

	
	if(GetAnimCount() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data setup is invalid. At least one source animation must be added before pre-processing."));
		bValidSetup = false;
	}

	if (!AreSequencesValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Data setup is invalid. There are null or incompatible animations in your Motion Data asset."));
		bValidSetup = false;
	}

	return bValidSetup;
}

bool UMotionDataAsset::AreSequencesValid()
{
	bool bValidAnims = true;

	const USkeleton* CompareSkeleton = MotionMatchConfig ? MotionMatchConfig->GetSourceSkeleton() : nullptr;

	for (const FMotionAnimSequence& MotionAnim : SourceMotionAnims)
	{
		if (MotionAnim.Sequence == nullptr
			|| !MotionAnim.Sequence->GetSkeleton()->IsCompatible(CompareSkeleton))
		{
			
			bValidAnims = false;
			break;
		}
	}

	for (const FMotionComposite& MotionComposite : SourceComposites)
	{
		if (MotionComposite.AnimComposite == nullptr
			|| !MotionComposite.AnimComposite->GetSkeleton()->IsCompatible(CompareSkeleton))
		{

			bValidAnims = false;
			break;
		}
	}

	for (const FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
		if (MotionBlendSpace.BlendSpace == nullptr
			|| !MotionBlendSpace.BlendSpace->GetSkeleton()->IsCompatible(CompareSkeleton))
		{
			bValidAnims = false;
			break;
		}
	}

	if (!bValidAnims)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid Animations: Null or incompatible animations found in motion data asset."))
	}

	return bValidAnims;
}


FDistanceMatchGroup& UMotionDataAsset::GetDistanceMatchGroup(const EDistanceMatchType MatchType, const EDistanceMatchBasis MatchBasis)
{
	return DistanceMatchSections[FDistanceMatchIdentifier(MatchType, MatchBasis)];
}

FDistanceMatchGroup& UMotionDataAsset::GetDistanceMatchGroup(const FDistanceMatchIdentifier MatchGroupIdentifier)
{
	return DistanceMatchSections[MatchGroupIdentifier];
}

void UMotionDataAsset::AddDistanceMatchSection(const FDistanceMatchSection& NewDistanceMatchSection)
{
	const FDistanceMatchIdentifier DistMatchId = FDistanceMatchIdentifier(NewDistanceMatchSection);
	
	DistanceMatchSections.FindOrAdd(DistMatchId).DistanceMatchSections.Add(NewDistanceMatchSection);
}

void UMotionDataAsset::AddAction(const FPoseMotionData& ClosestPose, const FMotionAnimAsset& MotionAnim, const int32 ActionId, const float Time)
{
	if (ActionId > -1)
	{
		int32 LastActionId = -1;
		
		//Action Sequencing
		if(Actions.Num() > 0)
		{
			//Find the last action
			FMotionAction& LastAction = Actions.Last();
			const FPoseMotionData& LastActionPose = Poses[LastAction.PoseId];

			//Only use the last action if it is within the same animation.
			if (LastActionPose.AnimId == MotionAnim.AnimId
				&& LastActionPose.AnimType == LastActionPose.AnimType
				&& LastActionPose.bMirrored == ClosestPose.bMirrored)
			{
				LastAction.NextActionId = Actions.Num();
				LastActionId = Actions.Num() -1;
			}
		}

		Actions.Emplace(FMotionAction(ActionId, ClosestPose.PoseId, Time, ClosestPose.Traits, LastActionId, -1));
	}
}

float UMotionDataAsset::GetPoseInterval() const
{
	return PoseInterval;
}

bool UMotionDataAsset::IsOptimisationValid() const
{
	return bOptimize && OptimisationModule != nullptr && OptimisationModule->IsProcessedAndValid(this);
}

void UMotionDataAsset::PostLoad()
{
	Super::Super::PostLoad();

	for (FMotionAnimAsset& MotionAnim : SourceMotionAnims)
	{
		MotionAnim.ParentMotionDataAsset = this;
	}

	for (FMotionAnimAsset& MotionComposite : SourceComposites)
	{
		MotionComposite.ParentMotionDataAsset = this;
	}

	for (FMotionAnimAsset& MotionBlendSpace : SourceBlendSpaces)
	{
		MotionBlendSpace.ParentMotionDataAsset = this;
	}
}

void UMotionDataAsset::Serialize(FArchive& Ar)
{
	Super::Super::Serialize(Ar);
}

#if WITH_EDITOR
void UMotionDataAsset::RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces)
{
	//No remapping required (individual animations need to be remapped
}
#endif

void UMotionDataAsset::TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const
{
	const float DeltaTime = Context.GetDeltaTime();
	const bool bGenerateNotifies = NotifyTriggerMode != ENotifyTriggerMode::None;

	TArray<FAnimNotifyEventReference> Notifies;
	
	const TArray<FAnimChannelState>* BlendChannels = reinterpret_cast<TArray<FAnimChannelState>*>(Instance.BlendSpace.BlendSampleDataCache);
	
	float HighestWeight = -1.0f;
	int32 HighestWeightChannelId = -1;
	for (int32 i = 0; i < BlendChannels->Num(); ++i)
	{
		const FAnimChannelState& ChannelState = (*BlendChannels)[i];

		if (ChannelState.BlendStatus != EBlendStatus::Inactive
			&& ChannelState.Weight > ZERO_ANIMWEIGHT_THRESH)
		{
			float ChannelWeight;
			
			switch (ChannelState.AnimType)
			{
				case EMotionAnimAssetType::Sequence: { ChannelWeight = TickAnimChannelForSequence(ChannelState, Context, Notifies, HighestWeight, DeltaTime, bGenerateNotifies); } break;
				case EMotionAnimAssetType::BlendSpace: { ChannelWeight = TickAnimChannelForBlendSpace(ChannelState, Context, Notifies, HighestWeight, DeltaTime, bGenerateNotifies); } break;
				case EMotionAnimAssetType::Composite: { ChannelWeight = TickAnimChannelForComposite(ChannelState, Context, Notifies, HighestWeight, DeltaTime, bGenerateNotifies); } break;
				default: { continue; } break;
			}

			if (ChannelWeight > HighestWeight)
			{
				HighestWeight = ChannelState.Weight;
				HighestWeightChannelId = i;
			}
		}
	}

	if (bGenerateNotifies)
	{
		if (NotifyTriggerMode == ENotifyTriggerMode::HighestWeightedAnimation)
		{
			const FAnimChannelState& ChannelState = (*BlendChannels)[HighestWeightChannelId];
			float PreviousTime = ChannelState.AnimTime - DeltaTime;

			switch (ChannelState.AnimType)
			{
				case EMotionAnimAssetType::Sequence: 
				{
					const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState.AnimId);

					if (!MotionAnim.bLoop)
					{
						const float AnimLength = MotionAnim.GetPlayLength();
						if (PreviousTime + DeltaTime > AnimLength)
						{
							PreviousTime = AnimLength - DeltaTime;
						}
					}
					
					FAnimTickRecord TickRecord;
					TickRecord.bLooping = MotionAnim.bLoop;
					FAnimNotifyContext AnimNotifyContext(TickRecord);
					MotionAnim.Sequence->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);
					Notifies = AnimNotifyContext.ActiveNotifies;
				} break;
				case EMotionAnimAssetType::BlendSpace:
				{
					const FMotionBlendSpace& MotionBlendSpace = GetSourceBlendSpaceAtIndex(ChannelState.AnimId);
					const bool bLooping = MotionBlendSpace.bLoop;

					float HighestSampleWeight = -1.0f;
					float HighestSampleId = 0;
					for (int32 k = 0; k < ChannelState.BlendSampleDataCache.Num(); ++k)
					{
						const FBlendSampleData& BlendSampleData = ChannelState.BlendSampleDataCache[k];
						const float SampleWeight = BlendSampleData.GetClampedWeight();
						
						if (SampleWeight > HighestSampleWeight)
						{
							HighestSampleWeight = SampleWeight;
							HighestSampleId = k;
						}
					}

					UAnimSequence* BlendSequence = ChannelState.BlendSampleDataCache[HighestSampleId].Animation;

					if (!bLooping)
					{
						if (PreviousTime + DeltaTime > BlendSequence->GetPlayLength())
						{
							PreviousTime = BlendSequence->GetPlayLength() - DeltaTime;
						}
					}

					if (BlendSequence)
					{
						FAnimTickRecord TickRecord;
						TickRecord.bLooping = bLooping;
						FAnimNotifyContext AnimNotifyContext(TickRecord);
						BlendSequence->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);
						Notifies = AnimNotifyContext.ActiveNotifies;
					}

				} break;

				case EMotionAnimAssetType::Composite:
				{
					const FMotionComposite& MotionComposite = GetSourceCompositeAtIndex(ChannelState.AnimId);

					if (!MotionComposite.bLoop)
					{
						const float AnimLength = MotionComposite.GetPlayLength();
						if (PreviousTime + DeltaTime > AnimLength)
						{
							PreviousTime = AnimLength - DeltaTime;
						}
					}
						
					FAnimTickRecord TickRecord;
					TickRecord.bLooping = MotionComposite.bLoop;
					FAnimNotifyContext AnimNotifyContext(TickRecord);
					MotionComposite.AnimComposite->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);
					Notifies = AnimNotifyContext.ActiveNotifies;
				}
			default: break;
			}
		}

		if(Notifies.Num() > 0)
		{
			//WARNING: The below is a workaround needed for notifies to work. The commented out function below should be called instead 
			//but it cannot be as it is not exported from the Engine Module. Using it results in linker errors.
			AddAnimNotifiesToNotifyQueue(NotifyQueue, Notifies, Instance.EffectiveBlendWeight);
			
			//NotifyQueue.AddAnimNotifies(Notifies, Instance.EffectiveBlendWeight);
		}
	}
}

float UMotionDataAsset::TickAnimChannelForSequence(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context, 
	TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const
{
	float ChannelWeight = -100.0f;

	const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState.AnimId);
	UAnimSequence* Sequence = MotionAnim.Sequence;

	if (Sequence)
	{
		//const float& CurrentSampleDataTime = ChannelState.AnimTime;
		const float CurrentTime = FMath::Clamp(ChannelState.AnimTime, 0.0f, Sequence->GetPlayLength());
		const float PreviousTime = CurrentTime - DeltaTime;

		if (bGenerateNotifies)
		{
			if (NotifyTriggerMode == ENotifyTriggerMode::AllAnimations)
			{
				FAnimTickRecord TickRecord;
				TickRecord.bLooping = MotionAnim.bLoop;
				FAnimNotifyContext AnimNotifyContext(TickRecord);
				Sequence->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);
				Notifies = AnimNotifyContext.ActiveNotifies;
			}
			else
			{
				ChannelWeight = ChannelState.Weight;
			}
		}

		//Root Motion
		if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything && Sequence->bEnableRootMotion)
		{
			FTransform RootMotion = Sequence->ExtractRootMotion(PreviousTime, DeltaTime, MotionAnim.bLoop);

			if (ChannelState.bMirrored && MirroringProfile != nullptr)
			{
				RootMotion.Mirror(EAxis::X, EAxis::X);
			}

			Context.RootMotionMovementParams.AccumulateWithBlend(RootMotion, ChannelState.Weight);
		}
	}

	return ChannelWeight;
}

float UMotionDataAsset::TickAnimChannelForBlendSpace(const FAnimChannelState& ChannelState,
                                                     FAnimAssetTickContext& Context, TArray<FAnimNotifyEventReference>& Notifies,
                                                     const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const
{
	float ChannelWeight = -100.0f;
	
	const FMotionBlendSpace& MotionBlendSpace = GetSourceBlendSpaceAtIndex(ChannelState.AnimId);
	UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;
	const float PlayLength = MotionBlendSpace.GetPlayLength();
	
	if (BlendSpace)
	{
		const float CurrentTime = FMath::Clamp(ChannelState.AnimTime, 0.0f, PlayLength);
		const float PreviousTime = CurrentTime - DeltaTime;


		for(int32 i = 0; i < ChannelState.BlendSampleDataCache.Num(); ++i)
		{
			const FBlendSampleData& BlendSample = ChannelState.BlendSampleDataCache[i];
			const UAnimSequence* SampleSequence = BlendSample.Animation;
			
			if(!SampleSequence)
			{
				continue;
			}
			
			const float SampleWeight = BlendSample.GetClampedWeight();
			if(SampleWeight <= ZERO_ANIMWEIGHT_THRESH)
			{
				continue;
			}

			//Notifies
			if (bGenerateNotifies)
			{
				if (NotifyTriggerMode == ENotifyTriggerMode::AllAnimations)
				{
					FAnimTickRecord TickRecord;
					TickRecord.bLooping = MotionBlendSpace.bLoop;
					FAnimNotifyContext AnimNotifyContext(TickRecord);
					SampleSequence->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);
					Notifies = AnimNotifyContext.ActiveNotifies;
				}
			}

			//RootMotion
			if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything && SampleSequence->bEnableRootMotion)
			{
				FTransform RootMotion = SampleSequence->ExtractRootMotion(PreviousTime, DeltaTime, MotionBlendSpace.bLoop);
				
				if (ChannelState.bMirrored && MirroringProfile != nullptr)
				{
					RootMotion.Mirror(EAxis::X, EAxis::X);
				}
				
				Context.RootMotionMovementParams.AccumulateWithBlend(RootMotion, ChannelState.Weight * SampleWeight);
			}
			
			//UE_LOG(LogAnimation, Verbose, TEXT("%d. Blending Animations(%s) with %f weight at time %0.2f"), i+1, SampleSequence->GetName(), SampleWeight, BlendSample.Time);
		}

		if (bGenerateNotifies && NotifyTriggerMode == ENotifyTriggerMode::HighestWeightedAnimation)
		{
			ChannelWeight = ChannelState.Weight;
		}
	}
	
	return ChannelWeight;
}

float UMotionDataAsset::TickAnimChannelForComposite(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context, 
	TArray<FAnimNotifyEventReference>& Notifies, const float HighestWeight, const float DeltaTime, const bool bGenerateNotifies) const
{
	float ChannelWeight = -100.0f;

	const FMotionComposite& MotionComposite = GetSourceCompositeAtIndex(ChannelState.AnimId);
	UAnimComposite* Composite = MotionComposite.AnimComposite;

	if (Composite)
	{
		const float& CurrentSampleDataTime = ChannelState.AnimTime;
		const float CurrentTime = FMath::Clamp(ChannelState.AnimTime, 0.0f, Composite->GetPlayLength());
		const float PreviousTime = CurrentTime - DeltaTime;

		if (bGenerateNotifies)
		{
			if (NotifyTriggerMode == ENotifyTriggerMode::AllAnimations)
			{
				FAnimTickRecord TickRecord;
				TickRecord.bLooping = MotionComposite.bLoop;
				FAnimNotifyContext AnimNotifyContext(TickRecord);
				Composite->GetAnimNotifies(PreviousTime, DeltaTime, AnimNotifyContext);

				Notifies.Reset(AnimNotifyContext.ActiveNotifies.Num());

				for(FAnimNotifyEventReference NotifyRef : AnimNotifyContext.ActiveNotifies)
				{
					if(const FAnimNotifyEvent* Notify = NotifyRef.GetNotify())
					{
						Notifies.Add(NotifyRef);
					}
				}
			}
			else
			{
				ChannelWeight = ChannelState.Weight;
			}
		}

		//Root Motion
		if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything/* && Composite->bEnableRootMotion*/)
		{
			FRootMotionMovementParams RootMotionParams;
			Composite->ExtractRootMotionFromTrack(Composite->AnimationTrack, PreviousTime, PreviousTime + DeltaTime, RootMotionParams);
			FTransform RootMotion = RootMotionParams.GetRootMotionTransform();

			if (ChannelState.bMirrored && MirroringProfile != nullptr)
			{
				RootMotion.Mirror(EAxis::X, EAxis::X);
			}

			Context.RootMotionMovementParams.AccumulateWithBlend(RootMotion, ChannelState.Weight);
		}

		//UE_LOG(LogAnimation, Verbose, TEXT("%d. Blending animation(%s) with %f weight at time %0.2f"), i + 1, *Composite->GetName(), ChannelState.Weight, CurrentTime);
	}

	return ChannelWeight;
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

#if WITH_EDITOR
bool UMotionDataAsset::GetAllAnimationSequencesReferred(TArray<class UAnimationAsset*>& AnimationSequences, bool bRecursive)
{
	Super::GetAllAnimationSequencesReferred(AnimationSequences, bRecursive);

	//Sequences
	for (const FMotionAnimSequence& MotionAnim : SourceMotionAnims)
	{
		if (MotionAnim.Sequence)
		{
			MotionAnim.Sequence->HandleAnimReferenceCollection(AnimationSequences, bRecursive);
			
			//AnimationSequences.Add(MotionAnim.Sequence);
		}
	}

	//Blend Spaces
	for(const FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
		if(MotionBlendSpace.BlendSpace)
		{
			for(int i = 0; i < MotionBlendSpace.BlendSpace->GetNumberOfBlendSamples(); ++i)
			{
				const FBlendSample& BlendSample = MotionBlendSpace.BlendSpace->GetBlendSample(i);

				if(BlendSample.Animation)
				{
					BlendSample.Animation->HandleAnimReferenceCollection(AnimationSequences, bRecursive);
				}
			}
		}
	}

	//Composites
	for(const FMotionComposite& MotionComposite : SourceComposites)
	{
		if(MotionComposite.AnimComposite)
		{

			MotionComposite.AnimComposite->GetAllAnimationSequencesReferred(AnimationSequences, bRecursive);
		}
	}

	return (AnimationSequences.Num() > 0);
}

void UMotionDataAsset::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ReplacementMap)
{
	Super::ReplaceReferredAnimations(ReplacementMap);

	//Sequences
	for (const FMotionAnimSequence& MotionAnim : SourceMotionAnims)
	{
		UAnimSequence* const* ReplacementAsset = reinterpret_cast<UAnimSequence* const*>(ReplacementMap.Find(MotionAnim.Sequence));
		if(ReplacementAsset)
		{
			MotionAnim.AnimAsset = *ReplacementAsset;
			MotionAnim.Sequence = *ReplacementAsset;
			MotionAnim.Sequence->ReplaceReferredAnimations(ReplacementMap);
		}
	}

	//BlendSpaces
	for(const FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
		UBlendSpace* const* ReplacementAsset = reinterpret_cast<UBlendSpace* const*>(ReplacementMap.Find(MotionBlendSpace.BlendSpace));
		if(ReplacementAsset)
		{
			MotionBlendSpace.AnimAsset = *ReplacementAsset;
			MotionBlendSpace.BlendSpace = *ReplacementAsset;
		}
	}

	//Composites
	for(const FMotionComposite& MotionComposite : SourceComposites)
	{
		UAnimComposite* const* ReplacementAsset = reinterpret_cast<UAnimComposite* const*>(ReplacementMap.Find(MotionComposite.AnimComposite));
		if(ReplacementAsset)
		{
			MotionComposite.AnimAsset = *ReplacementAsset;
			MotionComposite.AnimComposite = *ReplacementAsset;
		}
	}
}
#endif

void UMotionDataAsset::MotionAnimMetaDataModified()
{
	if (MotionMetaWrapper == nullptr
		|| AnimMetaPreviewIndex < 0)
	{
		return;
	}

	Modify();
	switch (AnimMetaPreviewType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			if(AnimMetaPreviewIndex > SourceMotionAnims.Num())
				return;

			FMotionAnimSequence& AnimMetaData = SourceMotionAnims[AnimMetaPreviewIndex];
			AnimMetaData.bLoop = MotionMetaWrapper->bLoop;
			AnimMetaData.PlayRate = MotionMetaWrapper->PlayRate;	
			AnimMetaData.bEnableMirroring = MotionMetaWrapper->bEnableMirroring;
			AnimMetaData.bFlattenTrajectory = MotionMetaWrapper->bFlattenTrajectory;
			AnimMetaData.PastTrajectory = MotionMetaWrapper->PastTrajectory;
			AnimMetaData.FutureTrajectory = MotionMetaWrapper->FutureTrajectory;
			AnimMetaData.PrecedingMotion = MotionMetaWrapper->PrecedingMotion;
			AnimMetaData.FollowingMotion = MotionMetaWrapper->FollowingMotion;
			AnimMetaData.CostMultiplier = MotionMetaWrapper->CostMultiplier;
			AnimMetaData.TraitNames = MotionMetaWrapper->TraitNames;
		}
		break;

		case EMotionAnimAssetType::BlendSpace:
		{
			if (AnimMetaPreviewIndex > SourceBlendSpaces.Num())
				return;

			UMotionBlendSpaceMetaDataWrapper* BSWrapper = Cast<UMotionBlendSpaceMetaDataWrapper>(MotionMetaWrapper);
			FMotionBlendSpace& AnimMetaData = SourceBlendSpaces[AnimMetaPreviewIndex];
			AnimMetaData.bLoop = MotionMetaWrapper->bLoop;
			AnimMetaData.PlayRate = MotionMetaWrapper->PlayRate;
			AnimMetaData.bEnableMirroring = MotionMetaWrapper->bEnableMirroring;		
			AnimMetaData.bFlattenTrajectory = MotionMetaWrapper->bFlattenTrajectory;
			AnimMetaData.PastTrajectory = MotionMetaWrapper->PastTrajectory;
			AnimMetaData.FutureTrajectory = MotionMetaWrapper->FutureTrajectory;
			AnimMetaData.PrecedingMotion = MotionMetaWrapper->PrecedingMotion;
			AnimMetaData.FollowingMotion = MotionMetaWrapper->FollowingMotion;
			AnimMetaData.CostMultiplier = MotionMetaWrapper->CostMultiplier;
			AnimMetaData.TraitNames = MotionMetaWrapper->TraitNames;

			if(BSWrapper)
			{
				AnimMetaData.SampleSpacing = BSWrapper->SampleSpacing;
			}
		} break;

		case EMotionAnimAssetType::Composite:
		{
			if (AnimMetaPreviewIndex > SourceComposites.Num())
			{
				return;
			}

			FMotionComposite& AnimMetaData = SourceComposites[AnimMetaPreviewIndex];
			AnimMetaData.bLoop = MotionMetaWrapper->bLoop;
			AnimMetaData.PlayRate = MotionMetaWrapper->PlayRate;
			AnimMetaData.bEnableMirroring = MotionMetaWrapper->bEnableMirroring;
			AnimMetaData.bFlattenTrajectory = MotionMetaWrapper->bFlattenTrajectory;
			AnimMetaData.PastTrajectory = MotionMetaWrapper->PastTrajectory;
			AnimMetaData.FutureTrajectory = MotionMetaWrapper->FutureTrajectory;
			AnimMetaData.PrecedingMotion = MotionMetaWrapper->PrecedingMotion;
			AnimMetaData.FollowingMotion = MotionMetaWrapper->FollowingMotion;
			AnimMetaData.CostMultiplier = MotionMetaWrapper->CostMultiplier;
			AnimMetaData.TraitNames = MotionMetaWrapper->TraitNames;

		} break;
	default: ;
	}

	MarkPackageDirty();
}

bool UMotionDataAsset::SetAnimMetaPreviewIndex(EMotionAnimAssetType AssetType, int32 CurAnimId)
{
	if(CurAnimId < 0)
	{
		return false;
	}

	AnimMetaPreviewType = AssetType;
	AnimMetaPreviewIndex = CurAnimId;

	switch (AssetType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
			MotionMetaWrapper->ParentAsset = this;

			if (CurAnimId >= SourceMotionAnims.Num())
			{
				return false;
			}

			MotionMetaWrapper->SetProperties(&SourceMotionAnims[AnimMetaPreviewIndex]);

		} break;
		case EMotionAnimAssetType::BlendSpace:
		{
			MotionMetaWrapper = NewObject<UMotionBlendSpaceMetaDataWrapper>();
			MotionMetaWrapper->ParentAsset = this;

			if (CurAnimId >= SourceBlendSpaces.Num())
			{
				return false;
			}

			MotionMetaWrapper->SetProperties(&SourceBlendSpaces[AnimMetaPreviewIndex]);

		} break;
		case EMotionAnimAssetType::Composite:
		{
			MotionMetaWrapper = NewObject<UMotionAnimMetaDataWrapper>();
			MotionMetaWrapper->ParentAsset = this;

			if(CurAnimId >= SourceComposites.Num())
			{
				return false;
			}
				
			MotionMetaWrapper->SetProperties(&SourceComposites[AnimMetaPreviewIndex]);

		} break;
		default:
		{
			return false;
		}
	}

	return true;
}

void UMotionDataAsset::AddAnimNotifiesToNotifyQueue(FAnimNotifyQueue& NotifyQueue, TArray<FAnimNotifyEventReference>& Notifies, float InstanceWeight) const
{
	for (const FAnimNotifyEventReference& NotifyRef : Notifies)
	{
		const FAnimNotifyEvent* Notify = NotifyRef.GetNotify();
		if (Notify)
		{
			bool bPassesFiltering = false;

			switch (Notify->NotifyFilterType)
			{
				case ENotifyFilterType::NoFiltering: { bPassesFiltering = true; } break;
				case ENotifyFilterType::LOD: { bPassesFiltering = Notify->NotifyFilterLOD > NotifyQueue.PredictedLODLevel; } break;
				default: { ensure(false); } break; //Unknown Filter Type
			}

			const bool bPassesChanceOfTriggering = Notify->NotifyStateClass ? true : NotifyQueue.RandomStream.FRandRange(
					                                       0.0f, 1.0f) < Notify->NotifyTriggerChance;


			const bool bPassesDedicatedServerCheck = Notify->bTriggerOnDedicatedServer || !IsRunningDedicatedServer();
			if (bPassesDedicatedServerCheck && Notify->TriggerWeightThreshold < InstanceWeight && bPassesFiltering && bPassesChanceOfTriggering )
			{
				Notify->NotifyStateClass ? NotifyQueue.AnimNotifies.AddUnique(NotifyRef) : NotifyQueue.AnimNotifies.Add(NotifyRef);
			}
		}
	}
}

void UMotionDataAsset::PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror /*= false*/)
{
#if WITH_EDITOR
	FMotionAnimSequence& MotionAnim = SourceMotionAnims[SourceAnimIndex];
	UAnimSequence* Sequence = MotionAnim.Sequence;

	if (!Sequence)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to pre-process animation. The animation sequence is null and has been skipped. Check that all your animations are valid."));
		return;
	}

	Modify();

	MotionAnim.AnimId = SourceAnimIndex;

	const float AnimLength = Sequence->GetPlayLength();
	const float PlayRate = MotionAnim.GetPlayRate();
	float CurrentTime = 0.0f;
	const float TimeHorizon = MotionMatchConfig->TrajectoryTimes.Last() * PlayRate;

	const FMotionTraitField AnimTraitHandle = UMMBlueprintFunctionLibrary::CreateMotionTraitFieldFromArray(MotionAnim.TraitNames);

	if(PoseInterval < 0.01f)
	{
		PoseInterval = 0.01f;
	}

	const int32 StartPoseId = Poses.Num();
	while (CurrentTime <= AnimLength)
	{
		const int32 PoseId = Poses.Num();
		
		bool bDoNotUse = ((CurrentTime < TimeHorizon) && (MotionAnim.PastTrajectory == ETrajectoryPreProcessMethod::IgnoreEdges))
			|| ((CurrentTime > AnimLength - TimeHorizon) && (MotionAnim.FutureTrajectory == ETrajectoryPreProcessMethod::IgnoreEdges))
			? true : false;

		if(MotionAnim.bLoop)
		{
			bDoNotUse = false;
		}

		FVector RootVelocity;
		float RootRotVelocity;
		FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, Sequence, CurrentTime, PoseInterval * PlayRate);

		RootVelocity *= PlayRate;
		RootRotVelocity *= PlayRate;
		
		if (bMirror)
		{
			RootVelocity.X *= -1.0f;
			RootRotVelocity *= -1.0f;
		}

		const float PoseCostMultiplier = MotionAnim.CostMultiplier;

		FPoseMotionData NewPoseData = FPoseMotionData(PoseId, EMotionAnimAssetType::Sequence, 
			SourceAnimIndex, CurrentTime, PoseCostMultiplier, bDoNotUse, bMirror, 
			RootRotVelocity, RootVelocity, AnimTraitHandle);
		
		//Process trajectory for pose
		for (int32 i = 0; i < MotionMatchConfig->TrajectoryTimes.Num(); ++i)
		{
			FTrajectoryPoint Point;
			const float PointTime = MotionMatchConfig->TrajectoryTimes[i] * PlayRate;

			if (MotionAnim.bLoop)
			{
				FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(Point, Sequence, CurrentTime, PointTime);
			}
			else
			{
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
		for (int32 i = 0; i < MotionMatchConfig->PoseBones.Num(); ++i)
		{
			FJointData JointData;

			if (bMirror)
			{
				const FName BoneName = MotionMatchConfig->PoseBones[i].BoneName;
				FName MirrorBoneName = MirroringProfile->FindBoneMirror(BoneName);
				
				const int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(MirrorBoneName);

				FMMPreProcessUtils::ExtractJointData(JointData, Sequence, MirrorBoneIndex, CurrentTime, PoseInterval * PlayRate);
				JointData.Velocity *= PlayRate;

				JointData.Position.X *= -1.0f;
				JointData.Velocity.X *= -1.0f;
			}
			else
			{
				FMMPreProcessUtils::ExtractJointData(JointData, Sequence, MotionMatchConfig->PoseBones[i], CurrentTime, PoseInterval * PlayRate);
				JointData.Velocity *= PlayRate;
			}
			
			NewPoseData.JointData.Add(JointData);
		}
		
		Poses.Add(NewPoseData);
		CurrentTime += PoseInterval * PlayRate;
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionAnim.Tags)
	{
		UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass);
		if (TagSection)
		{
			const float TagStartTime = NotifyEvent.GetTriggerTime() / PlayRate;
			const float TagEndTime = TagStartTime + (NotifyEvent.GetDuration() / PlayRate);

			//Pre-process the tag itself
			TagSection->PreProcessTag(MotionAnim, this, TagStartTime, TagEndTime);

			//Find the range of poses affected by this tag
			int32 TagStartPoseId = StartPoseId + FMath::RoundHalfToEven(TagStartTime / PoseInterval);
			int32 TagEndPoseId = StartPoseId + FMath::RoundHalfToEven(TagEndTime / PoseInterval);

			TagStartPoseId = FMath::Clamp(TagStartPoseId, 0, Poses.Num() - 1);
			TagEndPoseId = FMath::Clamp(TagEndPoseId, 0, Poses.Num() - 1);
			
			//Apply the tags pre-processing to all poses in this range
			for (int32 PoseIndex = TagStartPoseId; PoseIndex < TagEndPoseId; ++PoseIndex)
			{
				TagSection->PreProcessPose(Poses[PoseIndex], MotionAnim, this, TagStartTime, TagEndTime);
			}

			continue; //Don't check for a tag point if we already know its a tag section
		}
		
		UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify);
		if (TagPoint)
		{
			const float TagTime = NotifyEvent.GetTriggerTime() / PlayRate;
			int32 TagClosestPoseId = StartPoseId + FMath::RoundHalfToEven(TagTime / PoseInterval);
			TagClosestPoseId = FMath::Clamp(TagClosestPoseId, 0, Poses.Num() - 1);

			TagPoint->PreProcessTag(Poses[TagClosestPoseId], MotionAnim, this, TagTime);
		}
	}

#endif
}

void UMotionDataAsset::PreProcessBlendSpace(const int32 SourceBlendSpaceIndex, const bool bMirror /*= false*/)
{
#if WITH_EDITOR
	FMotionBlendSpace& MotionBlendSpace = SourceBlendSpaces[SourceBlendSpaceIndex];
	UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;

	if (!BlendSpace)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to pre-process blend space. The animation blend space is null and has been skipped. Check that all your animations are valid."));
		return;
	}

	FMotionTraitField AnimTraitHandle = UMMBlueprintFunctionLibrary::CreateMotionTraitFieldFromArray(MotionBlendSpace.TraitNames);

	Modify();

	MotionBlendSpace.AnimId = SourceBlendSpaceIndex;

	//Determine initial values to begin pre-processing
	bool TwoDBlendSpace = Cast<UBlendSpace>(BlendSpace) == nullptr ? false : true;
	FBlendParameter XAxisParameter = BlendSpace->GetBlendParameter(0);
	float XAxisStart = XAxisParameter.Min;
	float XAxisEnd = XAxisParameter.Max;
	float XAxisStep = FMath::Abs((XAxisEnd - XAxisStart) * MotionBlendSpace.SampleSpacing.X);
	float YAxisStart = 0.0f;
	float YAxisEnd = 0.1f;
	float YAxisStep = 0.2f;

	if (TwoDBlendSpace)
	{
		FBlendParameter YAxisParameter = BlendSpace->GetBlendParameter(1);
		YAxisStart = YAxisParameter.Min;
		YAxisEnd = YAxisParameter.Max;
		YAxisStep = FMath::Abs((YAxisEnd - YAxisStart) * MotionBlendSpace.SampleSpacing.Y);
	}

	FVector BlendSpacePosition = FVector(XAxisStart, YAxisStart, 0.0f);

	const float AnimLength = MotionBlendSpace.GetPlayLength();
	const float PlayRate = MotionBlendSpace.GetPlayRate();
	float CurrentTime = 0.0f;
	
	if (PoseInterval < 0.01f)
	{
		PoseInterval = 0.01f;
	}

	const int32 StartPoseId = Poses.Num();

	for (float YAxisValue = YAxisStart; YAxisValue <= YAxisEnd; YAxisValue += YAxisStep)
	{
		BlendSpacePosition.Y = YAxisValue;

		for (float XAxisValue = XAxisStart; XAxisValue <= XAxisEnd; XAxisValue += XAxisStep)
		{
			BlendSpacePosition.X = XAxisValue;

			//Evaluate blend space sample data here
			TArray<FBlendSampleData> BlendSampleData;
			
			int32 CachedTriangulationIndex = -1; //Workaround. Caching for performance during pre-processing is not necessary
			BlendSpace->GetSamplesFromBlendInput(BlendSpacePosition, BlendSampleData, CachedTriangulationIndex, false);

			CurrentTime = 0.0f;
			while (CurrentTime <= AnimLength)
			{
				int32 PoseId = Poses.Num();

				FVector RootVelocity;
				float RootRotVelocity;
				FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, BlendSampleData, CurrentTime, PoseInterval * PlayRate);

				RootVelocity *= PlayRate;
				RootRotVelocity *= PlayRate;
				
				if (bMirror)
				{
					RootVelocity.X *= -1.0f;
					RootRotVelocity *= -1.0f;
				}

				float PoseCostMultiplier = MotionBlendSpace.CostMultiplier;

				FPoseMotionData NewPoseData = FPoseMotionData(PoseId, EMotionAnimAssetType::BlendSpace, SourceBlendSpaceIndex,
					CurrentTime, PoseCostMultiplier, false, bMirror, RootRotVelocity, RootVelocity, AnimTraitHandle);

				NewPoseData.BlendSpacePosition = FVector2D(BlendSpacePosition.X, BlendSpacePosition.Y);
				
				//Process Trajectory for Pose
				for (int32 i = 0; i < MotionMatchConfig->TrajectoryTimes.Num(); ++i)
				{
					FTrajectoryPoint Point;

					const float PointTime = MotionMatchConfig->TrajectoryTimes[i] * PlayRate;

					//Blend spaces must be looping
					if(MotionBlendSpace.bLoop)
					{
						FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(Point, BlendSampleData, CurrentTime, PointTime);
					}
					else
					{
						//float PointTime = MotionMatchConfig->TrajectoryTimes[i];

						if (PointTime < 0.0f)
						{
							//past Point
							FMMPreProcessUtils::ExtractPastTrajectoryPoint(Point, BlendSampleData, CurrentTime, PointTime,
								MotionBlendSpace.PastTrajectory, MotionBlendSpace.PrecedingMotion);
						}
						else
						{
							FMMPreProcessUtils::ExtractFutureTrajectoryPoint(Point, BlendSampleData, CurrentTime, PointTime,
								MotionBlendSpace.FutureTrajectory, MotionBlendSpace.FollowingMotion);
						}
					}

					if (MotionBlendSpace.bFlattenTrajectory)
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

				const FReferenceSkeleton& RefSkeleton = BlendSpace->GetSkeleton()->GetReferenceSkeleton();

				//Process joints for pose
				for (int32 i = 0; i < MotionMatchConfig->PoseBones.Num(); ++i)
				{
					FJointData JointData;

					if (bMirror)
					{
						FName BoneName = MotionMatchConfig->PoseBones[i].BoneName;
						FName MirrorBoneName = MirroringProfile->FindBoneMirror(BoneName);

						int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(MirrorBoneName);

						FMMPreProcessUtils::ExtractJointData(JointData, BlendSampleData, MirrorBoneIndex, CurrentTime, PoseInterval * PlayRate);
						JointData.Velocity *= PlayRate;

						JointData.Position.X *= -1.0f;
						JointData.Velocity.X *= -1.0f;
					}
					else
					{
						FMMPreProcessUtils::ExtractJointData(JointData, BlendSampleData, MotionMatchConfig->PoseBones[i], CurrentTime, PoseInterval * PlayRate);
						JointData.Velocity *= PlayRate;
					}

					NewPoseData.JointData.Add(JointData);
				}

				Poses.Add(NewPoseData);
				CurrentTime += PoseInterval * PlayRate;
			}
		}
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionBlendSpace.Tags)
	{
		UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass);
		if (TagSection)
		{
			const float TagStartTime = NotifyEvent.GetTriggerTime() / PlayRate;
			const float TagEndTime = TagStartTime + (NotifyEvent.GetDuration() / PlayRate);

			//Pre-process the tag itself
			TagSection->PreProcessTag(MotionBlendSpace, this, TagStartTime, TagStartTime + NotifyEvent.Duration);

			//Find the range of poses affected by this tag
			int32 TagStartPoseId = StartPoseId + FMath::RoundHalfToEven(TagStartTime / PoseInterval);
			int32 TagEndPoseId = StartPoseId + FMath::RoundHalfToEven(TagEndTime / PoseInterval);

			TagStartPoseId = FMath::Clamp(TagStartPoseId, 0, Poses.Num() - 1);
			TagEndPoseId = FMath::Clamp(TagEndPoseId, 0, Poses.Num() - 1);

			//Apply the tags pre-processing to all poses in this range
			for (int32 PoseIndex = TagStartPoseId; PoseIndex < TagEndPoseId; ++PoseIndex)
			{
				TagSection->PreProcessPose(Poses[PoseIndex], MotionBlendSpace, this, TagStartTime, TagEndTime);
			}

			continue; //Don't check for a tag point if we already know its a tag section
		}

		UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify);
		if (TagPoint)
		{
			const float TagTime = NotifyEvent.GetTriggerTime() / PlayRate;
			int32 TagClosestPoseId = StartPoseId + FMath::RoundHalfToEven(TagTime / PoseInterval);
			TagClosestPoseId = FMath::Clamp(TagClosestPoseId, 0, Poses.Num() - 1);

			TagPoint->PreProcessTag(Poses[TagClosestPoseId], MotionBlendSpace, this, TagTime);
		}
	}
	
#endif
}

void UMotionDataAsset::PreProcessComposite(const int32 SourceCompositeIndex, const bool bMirror /*= false*/)
{
#if WITH_EDITOR
	FMotionComposite& MotionComposite = SourceComposites[SourceCompositeIndex];
	UAnimComposite* Composite = MotionComposite.AnimComposite;

	if (!Composite)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to pre-process composite. The animation composite is null and has been skipped. Check that all your animations are valid."));
		return;
	}

	Modify();

	MotionComposite.AnimId = SourceCompositeIndex;

	const float AnimLength = Composite->GetPlayLength();
	const float PlayRate = MotionComposite.GetPlayRate();
	float CurrentTime = 0.0f;
	const float TimeHorizon = MotionMatchConfig->TrajectoryTimes.Last();

	const FMotionTraitField AnimTraitHandle = UMMBlueprintFunctionLibrary::CreateMotionTraitFieldFromArray(MotionComposite.TraitNames);

	if (PoseInterval < 0.01f)
	{
		PoseInterval = 0.05f;
	}

	const int32 StartPoseId = Poses.Num();
	while (CurrentTime <= AnimLength)
	{
		const int32 PoseId = Poses.Num();

		const bool bDoNotUse = ((CurrentTime < TimeHorizon) && (MotionComposite.PastTrajectory == ETrajectoryPreProcessMethod::IgnoreEdges))
		                       || ((CurrentTime > AnimLength - TimeHorizon) && (MotionComposite.FutureTrajectory == ETrajectoryPreProcessMethod::IgnoreEdges))
			                       ? true : false;

		FVector RootVelocity;
		float RootRotVelocity;
		FMMPreProcessUtils::ExtractRootVelocity(RootVelocity, RootRotVelocity, Composite, CurrentTime, PoseInterval * PlayRate);
		RootVelocity *= PlayRate;
		RootRotVelocity *= PlayRate;
		
		if (bMirror)
		{
			RootVelocity.X *= -1.0f;
			RootRotVelocity *= -1.0f;
		}

		const float PoseCostMultiplier = MotionComposite.CostMultiplier;

		FPoseMotionData NewPoseData = FPoseMotionData(PoseId, EMotionAnimAssetType::Composite,
			SourceCompositeIndex, CurrentTime, PoseCostMultiplier, bDoNotUse, bMirror,
			RootRotVelocity, RootVelocity, AnimTraitHandle);

		//Process trajectory for pose
		for (int32 i = 0; i < MotionMatchConfig->TrajectoryTimes.Num(); ++i)
		{
			FTrajectoryPoint Point;
			const float PointTime = MotionMatchConfig->TrajectoryTimes[i] * PlayRate;

			if (MotionComposite.bLoop)
			{
				FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(Point, Composite, CurrentTime, PointTime);
			}
			else
			{
				if (PointTime < 0.0f)
				{
					//past Point
					FMMPreProcessUtils::ExtractPastTrajectoryPoint(Point, Composite, CurrentTime, PointTime,
						MotionComposite.PastTrajectory, MotionComposite.PrecedingMotion);
				}
				else
				{
					FMMPreProcessUtils::ExtractFutureTrajectoryPoint(Point, Composite, CurrentTime, PointTime,
						MotionComposite.FutureTrajectory, MotionComposite.FollowingMotion);
				}
			}

			if (MotionComposite.bFlattenTrajectory)
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

		const FReferenceSkeleton& RefSkeleton = Composite->GetSkeleton()->GetReferenceSkeleton();

		//Process joints for pose
		for (int32 i = 0; i < MotionMatchConfig->PoseBones.Num(); ++i)
		{
			FJointData JointData;

			if (bMirror)
			{
				const FName BoneName = MotionMatchConfig->PoseBones[i].BoneName;
				FName MirrorBoneName = MirroringProfile->FindBoneMirror(BoneName);

				const int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(MirrorBoneName);

				FMMPreProcessUtils::ExtractJointData(JointData, Composite, MirrorBoneIndex, CurrentTime, PoseInterval * PlayRate);
				JointData.Velocity *= PlayRate;
				
				JointData.Position.X *= -1.0f;
				JointData.Velocity.X *= -1.0f;
			}
			else
			{
				FMMPreProcessUtils::ExtractJointData(JointData, Composite, MotionMatchConfig->PoseBones[i], CurrentTime, PoseInterval * PlayRate);
				JointData.Velocity *= PlayRate;
			}

			NewPoseData.JointData.Add(JointData);
		}
		
		Poses.Add(NewPoseData);
		CurrentTime += PoseInterval * PlayRate;
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionComposite.Tags)
	{
		UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass);
		if (TagSection)
		{
			const float TagStartTime = NotifyEvent.GetTriggerTime() / PlayRate;
			const float TagEndTime = TagStartTime + (NotifyEvent.GetDuration() / PlayRate);

			//Pre-process the tag itself
			TagSection->PreProcessTag(MotionComposite, this, TagStartTime, TagStartTime + NotifyEvent.Duration);

			//Find the range of poses affected by this tag
			int32 TagStartPoseId = StartPoseId + FMath::RoundHalfToEven(TagStartTime / PoseInterval);
			int32 TagEndPoseId = StartPoseId + FMath::RoundHalfToEven(TagEndTime / PoseInterval);

			TagStartPoseId = FMath::Clamp(TagStartPoseId, 0, Poses.Num() - 1);
			TagEndPoseId = FMath::Clamp(TagEndPoseId, 0, Poses.Num() - 1);

			//Apply the tags pre-processing to all poses in this range
			for (int32 PoseIndex = TagStartPoseId; PoseIndex < TagEndPoseId; ++PoseIndex)
			{
				TagSection->PreProcessPose(Poses[PoseIndex], MotionComposite, this, TagStartTime, TagEndTime);
			}

			continue; //Don't check for a tag point if we already know its a tag section
		}

		UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify);
		if (TagPoint)
		{
			const float TagTime = NotifyEvent.GetTriggerTime() / PlayRate;
			int32 TagClosestPoseId = StartPoseId + FMath::RoundHalfToEven(TagTime / PoseInterval);
			TagClosestPoseId = FMath::Clamp(TagClosestPoseId, 0, Poses.Num() - 1);

			TagPoint->PreProcessTag(Poses[TagClosestPoseId], MotionComposite, this, TagTime);
		}
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
		
		if (BeforePose.AnimType == Pose.AnimType 
			&& BeforePose.AnimId == Pose.AnimId
			&& BeforePose.bMirrored == Pose.bMirrored
			&& FVector2D::Distance(BeforePose.BlendSpacePosition, Pose.BlendSpacePosition) < 0.001f)
		{
			Pose.LastPoseId = BeforePose.PoseId;
		}
		else
		{
			const FMotionAnimAsset* MotionAnim = GetSourceAnim(Pose.AnimId, Pose.AnimType);

			//If the animation is looping, the last Pose needs to wrap to the end
			if (MotionAnim->bLoop)
			{
				
				for(int32 n = Pose.PoseId + 1; n < Poses.Num(); ++n)
				{
					const FPoseMotionData& CandidatePose = Poses[n];

					if(CandidatePose.AnimType != Pose.AnimType
						|| CandidatePose.AnimId != Pose.AnimId
						|| CandidatePose.bMirrored != Pose.bMirrored)
					{
						Pose.LastPoseId = Poses[n-1].PoseId;
						break;
					}
							
				}
			}
			else
			{
				Pose.LastPoseId = Pose.PoseId;
			}
		}

		if (AfterPose.AnimType == Pose.AnimType
			&& AfterPose.AnimId == Pose.AnimId 
			&& AfterPose.bMirrored == Pose.bMirrored
			&& FVector2D::Distance(AfterPose.BlendSpacePosition, Pose.BlendSpacePosition) < 0.001f)
		{
			Pose.NextPoseId = AfterPose.PoseId;
		}
		else
		{
			const FMotionAnimAsset* MotionAnim = GetSourceAnim(Pose.AnimId, Pose.AnimType);
			
			if (MotionAnim->bLoop)
			{
				for(int32 n = Pose.PoseId - 1; n > -1; --n)
				{
					const FPoseMotionData& CandidatePose = Poses[n];

					if(CandidatePose.AnimType != Pose.AnimType
						|| CandidatePose.AnimId != Pose.AnimId
						|| CandidatePose.bMirrored != Pose.bMirrored)
					{
						Pose.NextPoseId = Poses[n+1].PoseId;
						break;
					}
				}
				
			}
			else
			{
				Pose.NextPoseId = Pose.PoseId;
			}
		}

		//If the Pose at the beginning of the database is looping, we need to fix its before Pose reference
		FPoseMotionData& StartPose = Poses[0];
		const FMotionAnimAsset* StartMotionAnim = GetSourceAnim(StartPose.AnimId, StartPose.AnimType);

		if (StartMotionAnim->bLoop)
		{
			const int32 PosesToEnd = FMath::FloorToInt((StartMotionAnim->GetPlayLength() - StartPose.Time) / PoseInterval);
			StartPose.LastPoseId = StartPose.PoseId + PosesToEnd;
		}

		//If the Pose at the end of the database is looping, we need to fix its after Pose reference
		FPoseMotionData& EndPose = Poses.Last();
		const FMotionAnimAsset* EndMotionAnim = GetSourceAnim(EndPose.AnimId, EndPose.AnimType);

		if (EndMotionAnim->bLoop)
		{
			const int32 PosesToBeginning = FMath::FloorToInt(EndPose.Time / PoseInterval);
			EndPose.NextPoseId = EndPose.PoseId - PosesToBeginning;
		}
	}
}


#undef LOCTEXT_NAMESPACE