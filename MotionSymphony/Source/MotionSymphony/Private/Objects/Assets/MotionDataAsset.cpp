//Copyright 2020-2023 Kenneth Claassen. All Rights Reserved.

#include "Objects/Assets/MotionDataAsset.h"
#include "Utility/MotionMatchingUtils.h"
#include "Data/MotionAnimMetaDataWrapper.h"
#include "Utility/MMPreProcessUtils.h"
#include "Data/AnimChannelState.h"
#include "Animation/AnimNotifyQueue.h"
#include "Misc/ScopedSlowTask.h"
#include "Animation/BlendSpace.h"
#include "Tags/TagSection.h"
#include "Tags/TagPoint.h"
#include "Utility/MMBlueprintFunctionLibrary.h"
#include "Animation/MirrorDataTable.h"

#if WITH_EDITOR
#include "AnimationEditorUtils.h"
#include "Misc/MessageDialog.h"
#endif


#define LOCTEXT_NAMESPACE "MotionPreProcessEditor"

UMotionDataAsset::UMotionDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	PoseInterval(0.1f),
	MotionMatchConfig(nullptr),
	JointVelocityCalculationMethod(EJointVelocityCalculationMethod::BodyDependent),
	NotifyTriggerMode(ENotifyTriggerMode::HighestWeightedAnimation),
	PreprocessCalibration(nullptr),
	bIsProcessed(false),
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

bool UMotionDataAsset::IsValidSourceAnimIndex(const int32 AnimIndex) const
{
	return SourceMotionAnims.IsValidIndex(AnimIndex);
}

bool UMotionDataAsset::IsValidSourceBlendSpaceIndex(const int32 BlendSpaceIndex)
{
	return SourceBlendSpaces.IsValidIndex(BlendSpaceIndex);
}

bool UMotionDataAsset::IsValidSourceCompositeIndex(const int32 CompositeIndex) const
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

	//Check that the motion matching config setup is valid
	if (!MotionMatchConfig && !MotionMatchConfig->IsSetupValidForMotionMatching())
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData PreProcess Validity Check Failed: Missing MotionMatchConfig reference."));
		return false;
	}


	//Check that there is at least one animation to pre-process
	const int32 SourceAnimCount = GetSourceAnimCount() + GetSourceBlendSpaceCount() + GetSourceCompositeCount();
	if (SourceAnimCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData PreProcess Validity Check Failed: No animations added"));
		bValid = false;
	}
	
	if (!bValid)
	{
#if WITH_EDITOR
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("Invalid MotionData",
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
	InitializePoseMatrix();
	
	MMPreProcessTask.EnterProgressFrame();

	//Animation Sequences
	for (int32 i = 0; i < SourceMotionAnims.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();

		PreProcessAnim(i, false);

		if (MirrorDataTable != nullptr && SourceMotionAnims[i].bEnableMirroring)
		{
			PreProcessAnim(i, true);
		}
	}

	//Blend Spaces
	for (int32 i = 0; i < SourceBlendSpaces.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();

		PreProcessBlendSpace(i, false);

		if(MirrorDataTable != nullptr && SourceBlendSpaces[i].bEnableMirroring)
		{
			PreProcessBlendSpace(i, true);
		}
	}

	//Composites
	for (int32 i = 0; i < SourceComposites.Num(); ++i)
	{
		MMPreAnimAnalyseTask.EnterProgressFrame();
		
		PreProcessComposite(i, false);

		if (MirrorDataTable != nullptr && SourceComposites[i].bEnableMirroring)
		{
			PreProcessComposite(i, true);
		}
	}
	
	GeneratePoseSequencing();
	MarkEdgePoses(0.25f);
	
	//Find a list of traits used
	TArray<FMotionTraitField> UsedMotionTraits;
	for (int32 i = 0; i < Poses.Num(); ++i)
	{
		UsedMotionTraits.AddUnique(Poses[i].Traits);
	}
	
	TraitMatrixMap.Empty(UsedMotionTraits.Num() + 1);
	for(FMotionTraitField Trait : UsedMotionTraits)
	{
		TraitMatrixMap.Add(Trait, FPoseMatrixSection());
	}
	
	GenerateSearchPoseMatrix();

	MMPreProcessTask.EnterProgressFrame();

	//Standard deviations
	FeatureStandardDeviations.Empty(UsedMotionTraits.Num());
	for (const FMotionTraitField& MotionTrait : UsedMotionTraits)
	{
		FCalibrationData& NewCalibrationData = FeatureStandardDeviations.Add(MotionTrait, FCalibrationData(this));
		NewCalibrationData.GenerateStandardDeviationWeights(this, MotionTrait);
	}

	PreprocessCalibration->Initialize();
	PreprocessCalibration->MarkPackageDirty();
	bIsProcessed = true;

	MMPreProcessTask.EnterProgressFrame();
#endif
}

void UMotionDataAsset::ClearPoses()
{
	Poses.Empty();
	bIsProcessed = false;
}

bool UMotionDataAsset::IsSetupValid()
{
	bool bValidSetup = true;

	//Check if Config is setup properly
	if(!MotionMatchConfig || !MotionMatchConfig->IsSetupValidForMotionMatching())
	{
		UE_LOG(LogTemp, Error, TEXT("MotionData setup is invalid. MotionMatchConfig property is not setup correctly."));
		bValidSetup = false;
	}
	else
	{
		SetSkeleton(MotionMatchConfig->GetSourceSkeleton());

		//Check mirroring profile is valid
		if (MirrorDataTable)
		{
			if (!MirrorDataTable.Get()
				|| MirrorDataTable->Skeleton != MotionMatchConfig->SourceSkeleton)
			{
				UE_LOG(LogTemp, Error, TEXT("Motion Data setup is invalid. The Mirroring Profile is either invalid or not compatible with the motion match config (i.e. they don't use the same skeleton)"));
				bValidSetup = false;
			}
		}
	}

	//Check if calibration is setup properly
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
#if WITH_EDITOR
		if (MotionAnim.Sequence == nullptr
			|| !MotionAnim.Sequence->GetSkeleton()->IsCompatibleForEditor(CompareSkeleton))
#else
		if (MotionAnim.Sequence == nullptr)
#endif
		{
			
			bValidAnims = false;
			break;
		}
	}

	for (const FMotionComposite& MotionComposite : SourceComposites)
	{
#if WITH_EDITOR
		if (MotionComposite.AnimComposite == nullptr
			|| !MotionComposite.AnimComposite->GetSkeleton()->IsCompatibleForEditor(CompareSkeleton))
#else
		if (MotionComposite.AnimComposite == nullptr)
#endif
		{

			bValidAnims = false;
			break;
		}
	}

	for (const FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
#if WITH_EDITOR
		if (MotionBlendSpace.BlendSpace == nullptr
			|| !MotionBlendSpace.BlendSpace->GetSkeleton()->IsCompatibleForEditor(CompareSkeleton))

#else
		if (MotionBlendSpace.BlendSpace == nullptr)
#endif
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

float UMotionDataAsset::GetPoseInterval() const
{
	return PoseInterval;
}

float UMotionDataAsset::GetPoseFavour(const int32 PoseId) const
{
	//Pose Favour is stored as the first atom of a pose array

	const int32 FavourIndex = PoseId * LookupPoseMatrix.AtomCount;

	if(FavourIndex < 0 || FavourIndex >= LookupPoseMatrix.PoseArray.Num())
	{
		return 1.0f;
	}

	return LookupPoseMatrix.PoseArray[FavourIndex];
}

int32 UMotionDataAsset::GetTraitStartIndex(const FMotionTraitField& MotionTrait)
{
	return TraitMatrixMap[MotionTrait].StartIndex;
}

int32 UMotionDataAsset::GetTraitEndIndex(const FMotionTraitField& MotionTrait)
{
	return TraitMatrixMap[MotionTrait].EndIndex;
}

int32 UMotionDataAsset::MatrixPoseIdToDatabasePoseId(int32 MatrixPoseId) const
{
	if(MatrixPoseId < PoseIdRemap.Num())
	{
		return PoseIdRemap[MatrixPoseId];
	}
	
	//Todo: Failed, log here
	return 0;
}

int32 UMotionDataAsset::DatabasePoseIdToMatrixPoseId(int32 DatabasePoseId) const
{
	if(const int32* MatrixPoseId = PoseIdRemapReverse.Find(DatabasePoseId))
	{
		return *MatrixPoseId;
	}

	//Todo: Failed, log here
	return 0;
}

bool UMotionDataAsset::IsSearchPoseMatrixGenerated() const
{
	return SearchPoseMatrix.PoseCount > 0;
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
	
	FAnimChannelState* ChannelState = reinterpret_cast<FAnimChannelState*>(Instance.BlendSpace.BlendSampleDataCache);
	
	switch (ChannelState->AnimType)
	{
		case EMotionAnimAssetType::Sequence: { TickAnimChannelForSequence(*ChannelState, Context, Notifies, DeltaTime, bGenerateNotifies); } break;
		case EMotionAnimAssetType::BlendSpace: { TickAnimChannelForBlendSpace(*ChannelState, Context, Notifies, DeltaTime, bGenerateNotifies); } break;
		case EMotionAnimAssetType::Composite: { TickAnimChannelForComposite(*ChannelState, Context, Notifies, DeltaTime, bGenerateNotifies); } break;
		default: break;
	}
	
	if (bGenerateNotifies)
	{
		if (NotifyTriggerMode == ENotifyTriggerMode::HighestWeightedAnimation)
		{

			float PreviousTime = ChannelState->AnimTime - DeltaTime;

			switch (ChannelState->AnimType)
			{
				case EMotionAnimAssetType::Sequence: 
				{
					const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState->AnimId);

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
					const FMotionBlendSpace& MotionBlendSpace = GetSourceBlendSpaceAtIndex(ChannelState->AnimId);
					const bool bLooping = MotionBlendSpace.bLoop;

					float HighestSampleWeight = -1.0f;
					float HighestSampleId = 0;
					for (int32 k = 0; k < ChannelState->BlendSampleDataCache.Num(); ++k)
					{
						const FBlendSampleData& BlendSampleData = ChannelState->BlendSampleDataCache[k];
						const float SampleWeight = BlendSampleData.GetClampedWeight();
						
						if (SampleWeight > HighestSampleWeight)
						{
							HighestSampleWeight = SampleWeight;
							HighestSampleId = k;
						}
					}

					UAnimSequence* BlendSequence = ChannelState->BlendSampleDataCache[HighestSampleId].Animation;

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
					const FMotionComposite& MotionComposite = GetSourceCompositeAtIndex(ChannelState->AnimId);

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

void UMotionDataAsset::TickAnimChannelForSequence(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context,
                                                  TArray<FAnimNotifyEventReference>& Notifies, const float DeltaTime, const bool bGenerateNotifies) const
{
	const FMotionAnimSequence& MotionAnim = GetSourceAnimAtIndex(ChannelState.AnimId);

	if (UAnimSequence* Sequence = MotionAnim.Sequence)
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
		}

		//Root Motion
		if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything && Sequence->bEnableRootMotion)
		{
			FTransform RootMotion = Sequence->ExtractRootMotion(PreviousTime, DeltaTime, MotionAnim.bLoop);

			if (ChannelState.bMirrored && MirrorDataTable != nullptr)
			{
				RootMotion.Mirror(EAxis::X, EAxis::X);
			}

			Context.RootMotionMovementParams.Accumulate(RootMotion);
		}
	}
}

void UMotionDataAsset::TickAnimChannelForBlendSpace(const FAnimChannelState& ChannelState,
                                                    FAnimAssetTickContext& Context, TArray<FAnimNotifyEventReference>& Notifies,
                                                    const float DeltaTime, const bool bGenerateNotifies) const
{
	const FMotionBlendSpace& MotionBlendSpace = GetSourceBlendSpaceAtIndex(ChannelState.AnimId);
	const float PlayLength = MotionBlendSpace.GetPlayLength();
	
	if (MotionBlendSpace.BlendSpace)
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
				
				if (ChannelState.bMirrored && MirrorDataTable != nullptr)
				{
					RootMotion.Mirror(EAxis::X, EAxis::X);
				}
				
				Context.RootMotionMovementParams.AccumulateWithBlend(RootMotion, SampleWeight);
			}
		}
	}
}

void UMotionDataAsset::TickAnimChannelForComposite(const FAnimChannelState& ChannelState, FAnimAssetTickContext& Context,
                                                   TArray<FAnimNotifyEventReference>& Notifies, const float DeltaTime, const bool bGenerateNotifies) const
{
	const FMotionComposite& MotionComposite = GetSourceCompositeAtIndex(ChannelState.AnimId);
	if (UAnimComposite* Composite = MotionComposite.AnimComposite)
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
		}

		//Root Motion
		if (Context.RootMotionMode == ERootMotionMode::RootMotionFromEverything)
		{
			FRootMotionMovementParams RootMotionParams;
			Composite->ExtractRootMotionFromTrack(Composite->AnimationTrack, PreviousTime, PreviousTime + DeltaTime, RootMotionParams);
			FTransform RootMotion = RootMotionParams.GetRootMotionTransform();

			if (ChannelState.bMirrored && MirrorDataTable != nullptr)
			{
				RootMotion.Mirror(EAxis::X, EAxis::X);
			}

			Context.RootMotionMovementParams.Accumulate(RootMotion);
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
		if(UAnimSequence* const* ReplacementAsset = reinterpret_cast<UAnimSequence* const*>(ReplacementMap.Find(MotionAnim.Sequence)))
		{
			MotionAnim.AnimAsset = *ReplacementAsset;
			MotionAnim.Sequence = *ReplacementAsset;
			MotionAnim.Sequence->ReplaceReferredAnimations(ReplacementMap);
		}
	}

	//BlendSpaces
	for(const FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
		if(UBlendSpace* const* ReplacementAsset = reinterpret_cast<UBlendSpace* const*>(ReplacementMap.Find(MotionBlendSpace.BlendSpace)))
		{
			MotionBlendSpace.AnimAsset = *ReplacementAsset;
			MotionBlendSpace.BlendSpace = *ReplacementAsset;
		}
	}

	//Composites
	for(const FMotionComposite& MotionComposite : SourceComposites)
	{
		if(UAnimComposite* const* ReplacementAsset = reinterpret_cast<UAnimComposite* const*>(ReplacementMap.Find(MotionComposite.AnimComposite)))
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
		if (const FAnimNotifyEvent* Notify = NotifyRef.GetNotify())
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

void UMotionDataAsset::InitializePoseMatrix()
{
	if(!MotionMatchConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("UMotionDataAsset::InitializePoseMatrix - cannot initialize pose matrix as the motion match config is null."))
		return;
	}
	
	//Determine the atom count per pose in the pose matrix
	int32 AtomCount = 1; //Note: AtomCount starts at one because the first float is used for pose favour
	for(const UMatchFeatureBase* MatchFeature : MotionMatchConfig->Features)
	{
		if(MatchFeature)
		{
			AtomCount += MatchFeature->Size();
		}
	}
	LookupPoseMatrix.AtomCount = AtomCount;

	//Get Pose Count from sequences
	int32 PoseCount = 0;
	for(FMotionAnimSequence& MotionAnimSequence : SourceMotionAnims)
	{
		const int32 PoseCountThisAnim = FMath::FloorToInt32(MotionAnimSequence.GetPlayLength() / (PoseInterval * MotionAnimSequence.PlayRate)) + 1;
		PoseCount += MotionAnimSequence.bEnableMirroring ? PoseCountThisAnim * 2 : PoseCountThisAnim;
	}

	//Get pose count from composites
	for(FMotionComposite& MotionComposite : SourceComposites)
	{
		const int32 PoseCountThisAnim = FMath::FloorToInt32(MotionComposite.GetPlayLength() / (PoseInterval * MotionComposite.PlayRate)) + 1;
		PoseCount += MotionComposite.bEnableMirroring ? PoseCountThisAnim * 2 : PoseCountThisAnim;
	}

	//Get pose count from blend spaces
	for(FMotionBlendSpace& MotionBlendSpace : SourceBlendSpaces)
	{
		const FBlendParameter XAxisParameter = MotionBlendSpace.BlendSpace->GetBlendParameter(0);
		const FBlendParameter YAxisParameter = MotionBlendSpace.BlendSpace->GetBlendParameter(1);
		
		const int32 BSSampleCount = FMath::Floor((XAxisParameter.Max - XAxisParameter.Min) / MotionBlendSpace.SampleSpacing.X)
			* FMath::Floor((YAxisParameter.Max - XAxisParameter.Min) / MotionBlendSpace.SampleSpacing.Y);

		const int32 PoseCountThisAnim = (FMath::FloorToInt32(MotionBlendSpace.GetPlayLength()
			/ (PoseInterval * MotionBlendSpace.PlayRate)) + 1) * BSSampleCount;
		
		PoseCount += MotionBlendSpace.bEnableMirroring ? PoseCountThisAnim * 2 : PoseCountThisAnim;
	}

	LookupPoseMatrix.PoseCount = PoseCount;
	LookupPoseMatrix.PoseArray.Empty(AtomCount * PoseCount + 1);
	LookupPoseMatrix.PoseArray.SetNumZeroed(AtomCount * PoseCount);
}

void UMotionDataAsset::PreProcessAnim(const int32 SourceAnimIndex, const bool bMirror /*= false*/)
{
#if WITH_EDITOR
	FMotionAnimSequence& MotionAnim = SourceMotionAnims[SourceAnimIndex];
	const UAnimSequence* Sequence = MotionAnim.Sequence;

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
	const float TimeHorizon = 1.0f * PlayRate;

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

		LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount] = 1.0f; //This is the pose favour, defaults to 1.0f and is set otherwise by tags
		
		int32 CurrentFeatureOffset = 1; //Current Feature offset starts at 1 because we need to skip the first float used for pose favour
		for(UMatchFeatureBase* MatchFeature : MotionMatchConfig->Features)
		{
			if(MatchFeature)
			{
				float* ResultLocation = &LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount + CurrentFeatureOffset];
				MatchFeature->EvaluatePreProcess(ResultLocation, MotionAnim.Sequence, CurrentTime, PoseInterval, bMirror, MirrorDataTable, &MotionAnim);
				
				CurrentFeatureOffset += MatchFeature->Size();
			}
		}
		
		Poses.Emplace(FPoseMotionData(PoseId, EMotionAnimAssetType::Sequence, SourceAnimIndex, CurrentTime,
			bDoNotUse ? EPoseSearchFlag::DoNotUse : EPoseSearchFlag::Searchable, bMirror, AnimTraitHandle));
		
		CurrentTime += PoseInterval * PlayRate;
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionAnim.Tags)
	{
		if (UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass))
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

		if (UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify))
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

	const FMotionTraitField AnimTraitHandle = UMMBlueprintFunctionLibrary::CreateMotionTraitFieldFromArray(MotionBlendSpace.TraitNames);

	Modify();

	MotionBlendSpace.AnimId = SourceBlendSpaceIndex;

	//Determine initial values to begin pre-processing
	const bool TwoDBlendSpace = Cast<UBlendSpace>(BlendSpace) == nullptr ? false : true;
	const FBlendParameter XAxisParameter = BlendSpace->GetBlendParameter(0);
	const float XAxisStart = XAxisParameter.Min;
	const float XAxisEnd = XAxisParameter.Max;
	const float XAxisStep = FMath::Abs((XAxisEnd - XAxisStart) * MotionBlendSpace.SampleSpacing.X);
	float YAxisStart = 0.0f;
	float YAxisEnd = 0.1f;
	float YAxisStep = 0.2f;

	if (TwoDBlendSpace)
	{
		const FBlendParameter YAxisParameter = BlendSpace->GetBlendParameter(1);
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
				const int32 PoseId = Poses.Num();
				
				LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount] = 1.0f; //This is the pose favour, defaults to 1.0f and is set otherwise by tags
		
				int32 CurrentFeatureOffset = 1; //Current Feature offset starts at 1 because we need to skip the first float used for pose favour
				for(UMatchFeatureBase* MatchFeature : MotionMatchConfig->Features)
				{
					if(MatchFeature)
					{
						float* ResultLocation = &LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount + CurrentFeatureOffset];
						MatchFeature->EvaluatePreProcess(ResultLocation, MotionBlendSpace.BlendSpace, CurrentTime, PoseInterval,
						                                 bMirror, MirrorDataTable, FVector2D(BlendSpacePosition.X, BlendSpacePosition.Y), &MotionBlendSpace);
						
						CurrentFeatureOffset += MatchFeature->Size();
					}
				}
				
				FPoseMotionData NewPoseData = FPoseMotionData(PoseId, EMotionAnimAssetType::BlendSpace, SourceBlendSpaceIndex,
					CurrentTime, EPoseSearchFlag::Searchable, bMirror, AnimTraitHandle);

				NewPoseData.BlendSpacePosition = FVector2D(BlendSpacePosition.X, BlendSpacePosition.Y);
				
				Poses.Add(NewPoseData);
				CurrentTime += PoseInterval * PlayRate;
			}
		}
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionBlendSpace.Tags)
	{
		if (UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass))
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

		if (UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify))
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
	const UAnimComposite* Composite = MotionComposite.AnimComposite;

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
	const float TimeHorizon = 1.0f * PlayRate;

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

		LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount] = 1.0f; //This is the pose favour, defaults to 1.0f and is set otherwise by tags
		
		int32 CurrentFeatureOffset = 1; //Current Feature offset starts at 1 because we need to skip the first float used for pose favour
		for(UMatchFeatureBase* MatchFeature : MotionMatchConfig->Features)
		{
			if(MatchFeature)
			{
				float* ResultLocation = &LookupPoseMatrix.PoseArray[PoseId * LookupPoseMatrix.AtomCount + CurrentFeatureOffset];
				MatchFeature->EvaluatePreProcess(ResultLocation, MotionComposite.AnimComposite, CurrentTime, PoseInterval,
				                                 bMirror, MirrorDataTable, &MotionComposite);
				CurrentFeatureOffset += MatchFeature->Size();
			}
		}
		
		Poses.Emplace(FPoseMotionData(PoseId, EMotionAnimAssetType::Composite, SourceCompositeIndex, CurrentTime,
			bDoNotUse ? EPoseSearchFlag::DoNotUse : EPoseSearchFlag::Searchable, bMirror, AnimTraitHandle));
		
		CurrentTime += PoseInterval * PlayRate;
	}

	//PreProcess Tags 
	for (FAnimNotifyEvent& NotifyEvent : MotionComposite.Tags)
	{
		if (UTagSection* TagSection = Cast<UTagSection>(NotifyEvent.NotifyStateClass))
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

		if (UTagPoint* TagPoint = Cast<UTagPoint>(NotifyEvent.Notify))
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

void UMotionDataAsset::MarkEdgePoses(float InMaxAnimBlendTime)
{
	const int32 EdgePoseCount = FMath::CeilToInt32(InMaxAnimBlendTime / GetPoseInterval());
	
	for(int32 i = 0; i < Poses.Num(); ++i)
	{
		if(Poses[i].SearchFlag == EPoseSearchFlag::DoNotUse)
		{
			//Look back a certain number of poses and mark them as edge poses
			for(int32 n = 1; n <= EdgePoseCount; ++n)
			{
				const int32 PoseIndex = i - n;
				if(PoseIndex < 0)
				{
					continue;
				}

				FPoseMotionData& Pose = Poses[PoseIndex];
				if(Pose.SearchFlag == EPoseSearchFlag::Searchable)
				{
					Pose.SearchFlag = EPoseSearchFlag::EdgePose;
				}
			}
		}
	}
}

void UMotionDataAsset::GenerateSearchPoseMatrix()
{
	//Find the total number of valid poses to search
	int32 ValidPoseCount = 0;
	for(int32 i = 0; i < Poses.Num(); ++i)
	{
		if(Poses[i].SearchFlag == EPoseSearchFlag::Searchable)
		{
			++ValidPoseCount;
		}
	}

	//Create the SearchPoseMatrix based on the number of valid poses. Prepare the remap arrays
	PoseIdRemap.SetNumZeroed(ValidPoseCount);
	PoseIdRemapReverse.Empty(ValidPoseCount+1);
	SearchPoseMatrix.AtomCount = LookupPoseMatrix.AtomCount;
	SearchPoseMatrix.PoseCount = ValidPoseCount;
	SearchPoseMatrix.PoseArray.SetNumZeroed(ValidPoseCount * LookupPoseMatrix.AtomCount);
	//Add valid pose Id remaps to the remap array and add poses to the search pose matrix
	int32 ValidPoseId = 0;

	for(TPair<FMotionTraitField, FPoseMatrixSection>& Pair : TraitMatrixMap)
	{
		Pair.Value.StartIndex = ValidPoseId;
		
		for(int32 i = 0; i < Poses.Num(); ++i)
		{
			const FPoseMotionData& Pose = Poses[i];

			if(Pose.SearchFlag != EPoseSearchFlag::Searchable
				|| Pose.Traits != Pair.Key)
			{
				continue;
			}

			//If the pose is valid we can copy it to the new pose array at the appropriate location
			const int32 BaseStartIndex = i * SearchPoseMatrix.AtomCount;
			const int32 ValidStartIndex = ValidPoseId * SearchPoseMatrix.AtomCount;
			for(int32 n = 0; n < SearchPoseMatrix.AtomCount; ++n)
			{
				SearchPoseMatrix.PoseArray[ValidStartIndex + n] = LookupPoseMatrix.PoseArray[BaseStartIndex + n];
			}

			//We need to add a valid pose id to the remap because the pose database now no longer matches the pose matrix
			PoseIdRemap[ValidPoseId] = i;
			PoseIdRemapReverse.Add(i, ValidPoseId);

			++ValidPoseId;
		}

		Pair.Value.EndIndex = ValidPoseId;
	}

	SearchPoseMatrix.PoseCount = ValidPoseId;

	//Create AABB data structures
	PoseAABBMatrix_Outer = FPoseAABBMatrix(SearchPoseMatrix, 64);
	PoseAABBMatrix_Inner = FPoseAABBMatrix(SearchPoseMatrix, 16);
}

#undef LOCTEXT_NAMESPACE
