// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_MSMotionMatching.h"
#include "AnimationRuntime.h"
#include "Animation/AnimSequence.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"

#include "Animation/AnimSyncScope.h"

static TAutoConsoleVariable<int32> CVarMMSearchDebug(
	TEXT("a.AnimNode.MoSymph.MMSearch.Debug"),
	0,
	TEXT("Turns Motion Matching Search Debugging On / Off.\n")
	TEXT("<=0: Off \n")
	TEXT("  1: On - Candidate Trajectory Debug\n")
	TEXT("  2: On - Optimisation Error Debugging\n"));

static TAutoConsoleVariable<int32> CVarMMTrajectoryDebug(
	TEXT("a.AnimNode.MoSymph.MMTrajectory.Debug"),
	0,
	TEXT("Turns Motion Matching Trajectory Debugging On / Off. \n")
	TEXT("<=0: Off \n")
	TEXT("  1: On - Show Desired Trajectory\n")
	TEXT("  2: On - Show Chosen Trajectory\n"));

static TAutoConsoleVariable<int32> CVarMMPoseDebug(
	TEXT("a.AnimNode.MoSymph.MMPose.Debug"),
	0,
	TEXT("Turns Motion Matching Pose Debugging On / Off. \n")
	TEXT("<=0: Off \n")
	TEXT("  1: On - Show Pose Position\n")
	TEXT("  2: On - Show Pose Position and Velocity"));

static TAutoConsoleVariable<int32> CVarMMAnimDebug(
	TEXT("a.AnimNode.MoSymph.MMAnim.Debug"),
	0,
	TEXT("Turns on animation debugging for Motion Matching On / Off. \n")
	TEXT("<=0: Off \n")
	TEXT("  2: On - Show Current Anim Info"));

FAnimNode_MSMotionMatching::FAnimNode_MSMotionMatching() :
	UpdateInterval(0.1f),
	PlaybackRate(1.0f),
	BlendTime(0.3f),
	OverrideQualityVsResponsivenessRatio(0.5f),
	bBlendOutEarly(true),
	PoseMatchMethod(EPoseMatchMethod::Optimized),
	TransitionMethod(ETransitionMethod::Inertialization),
	PastTrajectoryMode(EPastTrajectoryMode::ActualHistory),
	bBlendTrajectory(false),
	TrajectoryBlendMagnitude(1.0f),
	bFavourCurrentPose(false),
	CurrentPoseFavour(0.95f),
	bEnableToleranceTest(true),
	PositionTolerance(50.0f),
	RotationTolerance(2.0f),
	ActiveDistanceMatchSection(nullptr),
	DistanceMatchTime(0.0f),
	LastDistanceMatchKeyChecked(0),
	CurrentActionId(0),
	CurrentActionTime(0),
	CurrentActionEndTime(0),
	TimeSinceMotionUpdate(0.0f),
	TimeSinceMotionChosen(0.0f),
	PoseInterpolationValue(0.0f),
	bForcePoseSearch(false),
	CurrentChosenPoseId(0),
	DominantBlendChannel(0),
	bValidToEvaluate(false),
	bInitialized(false),
	bTriggerTransition(false), MotionMatchingMode(), AnimInstanceProxy(nullptr)
{
	DesiredTrajectory.Clear();
	BlendChannels.Empty(12);
	HistoricalPosesSearchCounts.Empty(11);

	for (int32 i = 0; i < 30; ++i)
	{
		HistoricalPosesSearchCounts.Add(0);
	}
}

FAnimNode_MSMotionMatching::~FAnimNode_MSMotionMatching()
{

}

void FAnimNode_MSMotionMatching::UpdateBlending(const float DeltaTime)
{
	float HighestBlendWeight = -1.0f;
	int32 HighestBlendChannel = 0;
	for (int32 i = 0; i < BlendChannels.Num(); ++i)
	{
		const bool bCurrent = i == BlendChannels.Num() - 1;
		
		const float Weight = BlendChannels[i].Update(DeltaTime, BlendTime, bCurrent, PlaybackRate);

		if (!bCurrent && Weight < -0.05f)
		{
			BlendChannels.RemoveAt(i);
			--i;
		}
		else if (Weight > HighestBlendWeight)
		{
			HighestBlendWeight = Weight;
			HighestBlendChannel = i;
		}
	}

	DominantBlendChannel = HighestBlendChannel;
}

void FAnimNode_MSMotionMatching::InitializeWithPoseRecorder(const FAnimationUpdateContext& Context)
{
	FAnimNode_MotionRecorder* MotionRecorderNode = nullptr;
	IMotionSnapper* MotionSnapper = Context.GetMessage<IMotionSnapper>();
	if (MotionSnapper)
	{
		MotionRecorderNode = &MotionSnapper->GetNode();
	}
	
	if(MotionRecorderNode)
	{
		MotionRecorderNode->RegisterBonesToRecord(GetMotionData()->MotionMatchConfig->PoseBones);
	}
	
	//Create the bone remap for runtime retargetting
#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
	USkeletalMesh* SkeletalMesh = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset();
#else
	USkeletalMesh* SkeletalMesh = Context.AnimInstanceProxy->GetSkelMeshComponent()->SkeletalMesh;
#endif
	if (!SkeletalMesh)
	{
		return;
	}

	const FReferenceSkeleton& AnimBPRefSkeleton = Context.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
	const FReferenceSkeleton& SkelMeshRefSkeleton = SkeletalMesh->GetRefSkeleton();
	
	UMotionMatchConfig* MMConfig = GetMotionData()->MotionMatchConfig;

	PoseBoneNames.Empty(MMConfig->PoseBones.Num() + 1);
	for(int32 i = 0; i < MMConfig->PoseBones.Num(); ++i)
	{
		PoseBoneNames.Add(MMConfig->PoseBones[i].BoneName);
	}
}

void FAnimNode_MSMotionMatching::InitializeMatchedTransition(const FAnimationUpdateContext& Context)
{
	TimeSinceMotionChosen = 0.0f;
	TimeSinceMotionUpdate = 0.0f;
	
	FAnimNode_MotionRecorder* MotionRecorderNode = nullptr;
	IMotionSnapper* MotionSnapper = Context.GetMessage<IMotionSnapper>();
	if (MotionSnapper)
	{
		MotionRecorderNode = &MotionSnapper->GetNode();
	}

	if (MotionRecorderNode)
	{
		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
		ScheduleTransitionPoseSearch(Context);
	}
	else
	{
		//We just jump to the default pose because there is no way to match to external nodes.
		JumpToPose(0);
	}
}

void FAnimNode_MSMotionMatching::InitializeDistanceMatching(const FAnimationUpdateContext& Context)
{
	if (bBlendTrajectory)
	{
		ApplyTrajectoryBlending();
	}

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	//Filter the appropriate Distance Match Groups
	FDistanceMatchGroup* DistanceMatchGroup = CurrentMotionData->DistanceMatchSections.Find(FDistanceMatchIdentifier(DistanceMatchPayload.MatchType, DistanceMatchPayload.MatchBasis));

	if(!DistanceMatchGroup || !FinalCalibrationSets.Contains(RequiredTraits))
	{
		return;
	}

	const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

	const float OverridePoseMultiplier = (1.0f - OverrideQualityVsResponsivenessRatio) * 2.0f;
	const float OverrideTrajMultiplier = OverrideQualityVsResponsivenessRatio * 2.0f;

	//Go through each section, find it's distance point and closest pose and find if it is the lowest cost
	FDistanceMatchSection* LowestCostSection = nullptr;
	float LowestCost = 10000000.0f;
	float LowestCostTime = -1.0f;
	int32 LowestCostPoseId = -1;
	int32 LowestLastKeyChecked = 0;
	for (FDistanceMatchSection& Section : DistanceMatchGroup->DistanceMatchSections)
	{
		int32 LastKeyChecked = 0;
		const float MatchTime = Section.FindMatchingTime(DistanceMatchPayload.MarkerDistance, LastKeyChecked);

		//Todo: Try interpolating the pose if matching is not great 
		const int32 PoseId = Section.StartPoseId + FMath::RoundToInt(MatchTime / CurrentMotionData->PoseInterval);
		FPoseMotionData& Pose = CurrentMotionData->Poses[PoseId];

		//Trajectory Cost
		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
		                                                         Pose.Trajectory, FinalCalibration) * OverrideTrajMultiplier;
		
		//Pose Cost
		Cost += FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData,
			Pose.JointData, FinalCalibration) * OverridePoseMultiplier;

		//Body Velocity Cost
		Cost += FVector::DistSquared(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity)
			* FinalCalibration.Weight_Momentum * OverridePoseMultiplier;

		//Rotational Momentum Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* FinalCalibration.Weight_AngularMomentum * OverridePoseMultiplier;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestCostTime = MatchTime;
			LowestCostSection = &Section;
			LowestCostPoseId = PoseId;
			LowestLastKeyChecked = LastKeyChecked;
		}
	}

	//Whatever pose at the desired distance has the lowest cost, use that pose for distance matching
	if (LowestCostSection)
	{
		ActiveDistanceMatchSection = LowestCostSection;
		MotionMatchingMode = EMotionMatchingMode::DistanceMatching;
		DistanceMatchTime = LowestCostTime;
		LastDistanceMatchKeyChecked = LowestLastKeyChecked;

		const FPoseMotionData& Pose = CurrentMotionData->Poses[LowestCostPoseId];

		TransitionToPose(LowestCostPoseId, Context, LowestCostTime - Pose.Time);
	}
}

void FAnimNode_MSMotionMatching::InitializeMotionAction(const FAnimationUpdateContext& Context)
{
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	//Calculate how many poses prior to the action to use
	const int32 PoseOffsetToStart = FMath::Abs(FMath::RoundHalfFromZero(MotionActionPayload.LeadLength / CurrentMotionData->PoseInterval));

	if(!FinalCalibrationSets.Contains(RequiredTraits))
	{
		return;
	}

	const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

	//Cost function on action poses
	int32 BestPoseId = -1;
	int32 BestActionId = -1;
	float BestActionCost = 10000000.0f;
	for (int32 i = 0; i < CurrentMotionData->Actions.Num(); ++i)
	{
		const FMotionAction& MotionAction = CurrentMotionData->Actions[i];

		if (MotionAction.ActionId == MotionActionPayload.ActionId) //TODO: Support Traits?
		{
			const int32 PoseId = FMath::Clamp(MotionAction.PoseId - PoseOffsetToStart, 0, CurrentMotionData->Poses.Num());
			FPoseMotionData& Pose = CurrentMotionData->Poses[PoseId];

			float Cost = FMotionMatchingUtils::ComputePoseCost(Pose.JointData, CurrentInterpolatedPose.JointData, FinalCalibration);

			//TODO: Add momentum
			//TODO: Add rotational momentum
			//TODO: Add trajectory

			Cost *= Pose.Favour;

			if (Cost < BestActionCost)
			{
				BestActionCost = Cost;
				BestActionId = i;
				BestPoseId = Pose.PoseId;
			}
		}
	}

	if (BestActionId > -1)
	{
		TransitionToPose(BestPoseId, Context);
		MotionMatchingMode = EMotionMatchingMode::Action;
		CurrentActionId = BestActionId;
		CurrentActionTime = CurrentMotionData->Actions[BestActionId].Time;
		CurrentActionEndTime = CurrentActionTime + MotionActionPayload.TailLength;
		CurrentActionTime -= MotionActionPayload.LeadLength;
		
	}
}

void FAnimNode_MSMotionMatching::UpdateMotionMatchingState(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	if (DistanceMatchPayload.bTrigger && DistanceMatchPayload.MatchType != EDistanceMatchType::None)
	{
		InitializeDistanceMatching(Context);
	}
	else if (bTriggerTransition)
	{
		InitializeMatchedTransition(Context);
		bTriggerTransition = false;
	}
	else
	{
		UpdateMotionMatching(DeltaTime, Context);
		UpdateBlending(DeltaTime);
	}
}

void FAnimNode_MSMotionMatching::UpdateDistanceMatchingState(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	if (DistanceMatchPayload.MatchType == EDistanceMatchType::None)
	{
		MotionMatchingMode = EMotionMatchingMode::MotionMatching;
		InitializeMatchedTransition(Context);
	}
	else
	{
		if (!UpdateDistanceMatching(DeltaTime, Context))
		{
			MotionMatchingMode = EMotionMatchingMode::MotionMatching;
			UpdateMotionMatching(DeltaTime, Context);
			UpdateBlending(DeltaTime);
		}
	}
}

void FAnimNode_MSMotionMatching::UpdateMotionActionState(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	const float PlayRateAdjustedDeltaTime = DeltaTime * PlaybackRate;
	
	TimeSinceMotionChosen += PlayRateAdjustedDeltaTime;
	TimeSinceMotionUpdate += PlayRateAdjustedDeltaTime;
	CurrentActionTime += PlayRateAdjustedDeltaTime;

	UpdateBlending(DeltaTime);

	if (CurrentActionTime >= CurrentActionEndTime)
	{
		MotionMatchingMode = EMotionMatchingMode::MotionMatching;
	}
}

void FAnimNode_MSMotionMatching::UpdateMotionMatching(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	bForcePoseSearch = false;
	const float PlayRateAdjustedDeltaTime = DeltaTime * PlaybackRate;
	TimeSinceMotionChosen += PlayRateAdjustedDeltaTime;
	TimeSinceMotionUpdate += PlayRateAdjustedDeltaTime;

	const FAnimChannelState& PrimaryChannel = BlendChannels.Last();

	if (!PrimaryChannel.bLoop)
	{
		float CurrentBlendTime = 0.0f;

		if (bBlendOutEarly)
		{
			CurrentBlendTime = BlendTime * PrimaryChannel.Weight * PlaybackRate;
		}

		if (TimeSinceMotionChosen + PrimaryChannel.StartTime + CurrentBlendTime > PrimaryChannel.AnimLength)
		{
			bForcePoseSearch = true;
		}
	}
	FAnimNode_MotionRecorder* MotionRecorderNode = nullptr;
	IMotionSnapper* MotionSnapper = Context.GetMessage<IMotionSnapper>();
	if (MotionSnapper)
	{
		MotionRecorderNode = &MotionSnapper->GetNode();
	}
	
	if (MotionRecorderNode)
	{
		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
	}
	else
	{
		ComputeCurrentPose();
	}

	//If we have ran into a 'DoNotUse' pose. We need to force a new pose search
	if(CurrentInterpolatedPose.bDoNotUse)
	{
		bForcePoseSearch = true;
	}

	UMotionMatchConfig* MMConfig = GetMotionData()->MotionMatchConfig;

	//Past trajectory mode
	if (PastTrajectoryMode == EPastTrajectoryMode::CopyFromCurrentPose)
	{
		for (int32 i = 0; i < MMConfig->TrajectoryTimes.Num(); ++i)
		{
			if (MMConfig->TrajectoryTimes[i] > 0.0f)
			{ 
				break;
			}

			DesiredTrajectory.TrajectoryPoints[i] = CurrentInterpolatedPose.Trajectory[i];
		}
	}

	if (TimeSinceMotionUpdate >= UpdateInterval || bForcePoseSearch)
	{
		TimeSinceMotionUpdate = 0.0f;
		SchedulePoseSearch(Context);
	}
}

bool FAnimNode_MSMotionMatching::UpdateDistanceMatching(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	UpdateBlending(DeltaTime);

	DistanceMatchTime = ActiveDistanceMatchSection->FindMatchingTime(DistanceMatchPayload.MarkerDistance, LastDistanceMatchKeyChecked);

	FAnimChannelState& PrimaryChannel = BlendChannels.Last();
	PrimaryChannel.AnimTime = DistanceMatchTime;

	if (FMath::Abs(DistanceMatchTime - ActiveDistanceMatchSection->EndTime) < 0.01f)
	{
		return false;
	}

	return true;
}

void FAnimNode_MSMotionMatching::ComputeCurrentPose()
{
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	const float PoseInterval = FMath::Max(0.01f, CurrentMotionData->PoseInterval);

	//====== Determine the next chosen pose ========
	const FAnimChannelState& ChosenChannel = BlendChannels.Last();

	float ChosenClipLength = 0.0f;
	switch (ChosenChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: ChosenClipLength = CurrentMotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::BlendSpace: ChosenClipLength = CurrentMotionData->GetSourceBlendSpaceAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::Composite: ChosenClipLength = CurrentMotionData->GetSourceCompositeAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		default: ;
	}

	float TimePassed = TimeSinceMotionChosen;
	int32 PoseIndex = ChosenChannel.StartPoseId;

	float NewChosenTime = ChosenChannel.AnimTime;
	if (ChosenChannel.AnimTime >= ChosenClipLength)
	{
		if (ChosenChannel.bLoop)
		{
			NewChosenTime = FMotionMatchingUtils::WrapAnimationTime(NewChosenTime, ChosenClipLength);
		}
		else
		{
			const float TimeToNextClip = ChosenClipLength - (TimePassed + ChosenChannel.StartTime);

			if (TimeToNextClip < PoseInterval / 2.0f)
			{
				--PoseIndex;
			}

			NewChosenTime = ChosenClipLength;
		}

		TimePassed = NewChosenTime - ChosenChannel.StartTime;
	}

	int32 NumPosesPassed = 0;
	if (TimePassed < 0.0f)
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}
	else
	{
		NumPosesPassed = FMath::FloorToInt(TimePassed / PoseInterval); 
	}

	CurrentChosenPoseId = PoseIndex + NumPosesPassed;

	//====== Determine the next dominant pose ========
	const FAnimChannelState& DominantChannel = BlendChannels[DominantBlendChannel];

	float DominantClipLength = 0.0f;
	switch (ChosenChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: DominantClipLength = CurrentMotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::BlendSpace: DominantClipLength = CurrentMotionData->GetSourceBlendSpaceAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::Composite: DominantClipLength = CurrentMotionData->GetSourceCompositeAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		default: ;
	}

	if (TransitionMethod == ETransitionMethod::Blend)
	{
		TimePassed = DominantChannel.Age;
	}
	else
	{
		TimePassed = TimeSinceMotionChosen;
	}

	PoseIndex = DominantChannel.StartPoseId;

	//Determine if the new time is out of bounds of the dominant pose clip
	float NewDominantTime = DominantChannel.StartTime + TimePassed;
	if (NewDominantTime >= DominantClipLength)
	{
		if (DominantChannel.bLoop)
		{
			NewDominantTime = FMotionMatchingUtils::WrapAnimationTime(NewDominantTime, DominantClipLength);
		}
		else
		{
			const float TimeToNextClip = DominantClipLength - (TimePassed + DominantChannel.StartTime);

			if (TimeToNextClip < PoseInterval)
			{
				--PoseIndex;
			}

			NewDominantTime = DominantClipLength;
		}

		TimePassed = NewDominantTime - DominantChannel.StartTime;
	}

	if (TimePassed < -0.00001f)
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}
	else
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}

	PoseIndex = FMath::Clamp(PoseIndex + NumPosesPassed, 0, CurrentMotionData->Poses.Num());

	//Get the before and after poses and then interpolate
	FPoseMotionData* BeforePose;
	FPoseMotionData* AfterPose;

	if (TimePassed < -0.00001f)
	{
		AfterPose = &CurrentMotionData->Poses[PoseIndex];
		BeforePose = &CurrentMotionData->Poses[FMath::Clamp(AfterPose->LastPoseId, 0, CurrentMotionData->Poses.Num() - 1)];

		PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - (float)NumPosesPassed);
	}
	else
	{
		BeforePose = &CurrentMotionData->Poses[FMath::Min(PoseIndex, CurrentMotionData->Poses.Num() - 2)];
		AfterPose = &CurrentMotionData->Poses[BeforePose->NextPoseId];

		PoseInterpolationValue = (TimePassed / PoseInterval) - (float)NumPosesPassed;
	}

	FMotionMatchingUtils::LerpPose(CurrentInterpolatedPose, *BeforePose, *AfterPose, PoseInterpolationValue);
}

void FAnimNode_MSMotionMatching::ComputeCurrentPose(const FCachedMotionPose& CachedMotionPose)
{
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	const float PoseInterval = FMath::Max(0.01f, CurrentMotionData->PoseInterval);

	//====== Determine the next chosen pose ========
	const FAnimChannelState& ChosenChannel = BlendChannels.Last();

	float ChosenClipLength = 0.0f;
	switch (ChosenChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: ChosenClipLength = CurrentMotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::BlendSpace: ChosenClipLength = CurrentMotionData->GetSourceBlendSpaceAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::Composite: ChosenClipLength = CurrentMotionData->GetSourceCompositeAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
		default: ;
	}

	float TimePassed = TimeSinceMotionChosen;
	int32 PoseIndex = ChosenChannel.StartPoseId;

	float NewChosenTime = ChosenChannel.AnimTime;
	if (ChosenChannel.AnimTime >= ChosenClipLength)
	{
		if (ChosenChannel.bLoop)
		{
			NewChosenTime = FMotionMatchingUtils::WrapAnimationTime(NewChosenTime, ChosenClipLength);
		}
		else
		{
			const float TimeToNextClip = ChosenClipLength - (TimePassed + ChosenChannel.StartTime);

			if (TimeToNextClip < PoseInterval / 2.0f)
			{
				--PoseIndex;
			}

			NewChosenTime = ChosenClipLength;
		}

		TimePassed = NewChosenTime - ChosenChannel.StartTime;
	}

	int32 NumPosesPassed = 0;
	if (TimePassed < 0.0f)
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}
	else
	{
		NumPosesPassed = FMath::FloorToInt(TimePassed / PoseInterval);
	}
	
	CurrentChosenPoseId = PoseIndex + NumPosesPassed;

	//====== Determine the next dominant pose ========
	const FAnimChannelState& DominantChannel = BlendChannels[DominantBlendChannel];

	float DominantClipLength = 0.0f;
	switch (ChosenChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: DominantClipLength = CurrentMotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::BlendSpace: DominantClipLength = CurrentMotionData->GetSourceBlendSpaceAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		case EMotionAnimAssetType::Composite: DominantClipLength = CurrentMotionData->GetSourceCompositeAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
		default: ;
	}

	if (TransitionMethod == ETransitionMethod::Blend)
	{
		TimePassed = DominantChannel.Age;
	}
	else
	{
		TimePassed = TimeSinceMotionChosen;
	}

	PoseIndex = DominantChannel.StartPoseId;

	//Determine if the new time is out of bounds of the dominant pose clip
	float NewDominantTime = DominantChannel.StartTime + TimePassed;
	if (NewDominantTime >= DominantClipLength)
	{
		if (DominantChannel.bLoop)
		{
			NewDominantTime = FMotionMatchingUtils::WrapAnimationTime(NewDominantTime, DominantClipLength);
		}
		else
		{
			const float TimeToNextClip = DominantClipLength - (TimePassed + DominantChannel.StartTime);

			if (TimeToNextClip < PoseInterval)
			{
				--PoseIndex;
			}

			NewDominantTime = DominantClipLength;
		}

		TimePassed = NewDominantTime - DominantChannel.StartTime;
	}

	if (TimePassed < -0.00001f)
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}
	else
	{
		NumPosesPassed = FMath::CeilToInt(TimePassed / PoseInterval);
	}

	const int32 MaxPoseIndex = CurrentMotionData->Poses.Num() - 1;
	PoseIndex = FMath::Clamp(PoseIndex + NumPosesPassed, 0, MaxPoseIndex);

	//Get the before and after poses and then interpolate
	FPoseMotionData* BeforePose;
	FPoseMotionData* AfterPose;

	if (TimePassed < -0.00001f)
	{
		AfterPose = &CurrentMotionData->Poses[PoseIndex];
		BeforePose = &CurrentMotionData->Poses[FMath::Clamp(AfterPose->LastPoseId, 0, MaxPoseIndex)];

		PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - (float)NumPosesPassed);
	}
	else
	{
		BeforePose = &CurrentMotionData->Poses[FMath::Min(PoseIndex, CurrentMotionData->Poses.Num() - 2)];
		AfterPose = &CurrentMotionData->Poses[FMath::Clamp(BeforePose->NextPoseId, 0, MaxPoseIndex)];

		PoseInterpolationValue = (TimePassed / PoseInterval) - (float)NumPosesPassed;
	}
	
	FMotionMatchingUtils::LerpPoseTrajectory(CurrentInterpolatedPose, *BeforePose,
		*AfterPose, FMath::Clamp(PoseInterpolationValue, 0.0f, 1.0f));

	UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

	for(int32 i = 0; i < PoseBoneNames.Num(); ++i)
	{
		const FCachedMotionBone& CachedMotionBone = CachedMotionPose.CachedBoneData[PoseBoneNames[i]];
		CurrentInterpolatedPose.JointData[i] = FJointData(CachedMotionBone.Transform.GetLocation(), CachedMotionBone.Velocity);
	}
}

void FAnimNode_MSMotionMatching::SchedulePoseSearch(const FAnimationUpdateContext& Context)
{
	if (bBlendTrajectory)
	{
		ApplyTrajectoryBlending();
	}

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	const int32 MaxPoseId = CurrentMotionData->Poses.Num() - 1;
	CurrentChosenPoseId = FMath::Clamp(CurrentChosenPoseId, 0, MaxPoseId); //Just in case
	int32 NextPoseId = CurrentMotionData->Poses[CurrentChosenPoseId].NextPoseId;
	if(NextPoseId < 0)
	{
		NextPoseId = CurrentChosenPoseId;
	}
	
	FPoseMotionData& NextPose = CurrentMotionData->Poses[FMath::Clamp(NextPoseId, 0, MaxPoseId)];

	if (!bForcePoseSearch && bEnableToleranceTest)
	{
		if (NextPoseToleranceTest(NextPose))
		{
			TimeSinceMotionUpdate = 0.0f;
			return;
		}
	}

	int32 LowestPoseId = NextPose.PoseId;

	switch (PoseMatchMethod)
	{
		case EPoseMatchMethod::Optimized: { LowestPoseId = GetLowestCostPoseId(NextPose); } break;
		case EPoseMatchMethod::Linear: { LowestPoseId = GetLowestCostPoseId_Linear(NextPose); } break;
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	const int32 DebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();

	if(DebugLevel == 2)
	{
		PerformLinearSearchComparison(Context.AnimInstanceProxy, LowestPoseId, NextPose);
	}
#endif

	const FPoseMotionData& BestPose = CurrentMotionData->Poses[LowestPoseId];
	const FPoseMotionData& ChosenPose = CurrentMotionData->Poses[CurrentChosenPoseId];

	bool bWinnerAtSameLocation = BestPose.AnimId == CurrentInterpolatedPose.AnimId &&
								 BestPose.bMirrored == CurrentInterpolatedPose.bMirrored &&
								FMath::Abs(BestPose.Time - CurrentInterpolatedPose.Time) < 0.25f
								&& FVector2D::DistSquared(BestPose.BlendSpacePosition, CurrentInterpolatedPose.BlendSpacePosition) < 1.0f;

	

	if (!bWinnerAtSameLocation)
	{
		bWinnerAtSameLocation = BestPose.AnimId == ChosenPose.AnimId &&
								BestPose.bMirrored == ChosenPose.bMirrored &&
								FMath::Abs(BestPose.Time - ChosenPose.Time) < 0.25f
								&& FVector2D::DistSquared(BestPose.BlendSpacePosition, ChosenPose.BlendSpacePosition) < 1.0f;
	}

	if (!bWinnerAtSameLocation)
	{
		TransitionToPose(BestPose.PoseId, Context);
	}
}

void FAnimNode_MSMotionMatching::ScheduleTransitionPoseSearch(const FAnimationUpdateContext & Context)
{
	int32 LowestPoseId = GetLowestCostPoseId();

	LowestPoseId = FMath::Clamp(LowestPoseId, 0, GetMotionData()->Poses.Num() - 1);
	JumpToPose(LowestPoseId);
}

int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId()
{
	if (!FinalCalibrationSets.Contains(RequiredTraits))
	{
		return CurrentChosenPoseId;
	}

	const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : CurrentMotionData->Poses)
	{
		if (Pose.bDoNotUse 
		 && Pose.Traits != RequiredTraits)
		{
			continue;
		}

		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
		                                                         Pose.Trajectory, FinalCalibration);

		Cost += FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData,
			Pose.JointData, FinalCalibration);

		Cost += FVector::DistSquared(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * FinalCalibration.Weight_Momentum;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

	return LowestPoseId;
}

int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId(const FPoseMotionData& NextPose)
{
	if (!FinalCalibrationSets.Contains(RequiredTraits))
	{
		return CurrentChosenPoseId;
	}

	const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

	TArray<FPoseMotionData>* PoseCandidates = GetMotionData()->OptimisationModule->GetFilteredPoseList(CurrentInterpolatedPose, RequiredTraits, FinalCalibration);

	if (!PoseCandidates)
	{
		return GetLowestCostPoseId_Linear(NextPose);
	}

	const float OverridePoseMultiplier = (1.0f - OverrideQualityVsResponsivenessRatio) * 2.0f;
	const float OverrideTrajectoryMultiplier = OverrideQualityVsResponsivenessRatio * 2.0f;

	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : *PoseCandidates)
	{
		//Body Momentum
		float Cost = FVector::DistSquared(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity)
			* FinalCalibration.Weight_Momentum * OverridePoseMultiplier;

		//Body Rotational Momentum
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* FinalCalibration.Weight_AngularMomentum * OverridePoseMultiplier;

		if(Cost > LowestCost) 
		{
			continue; //Early Out
		}

		//Trajectory Cost 
		const int32 TrajectoryIterations = FMath::Min(DesiredTrajectory.TrajectoryPoints.Num(), FinalCalibration.TrajectoryWeights.Num());
		for (int32 i = 0; i < TrajectoryIterations; ++i)
		{
			const FTrajectoryWeightSet WeightSet = FinalCalibration.TrajectoryWeights[i];
			const FTrajectoryPoint CurrentPoint = DesiredTrajectory.TrajectoryPoints[i];
			const FTrajectoryPoint CandidatePoint = Pose.Trajectory[i];

			Cost += FVector::DistSquared(CandidatePoint.Position, CurrentPoint.Position) * WeightSet.Weight_Pos * OverrideTrajectoryMultiplier;
			Cost += FMath::Abs(FMath::FindDeltaAngleDegrees(CandidatePoint.RotationZ, CurrentPoint.RotationZ))
				* WeightSet.Weight_Facing * OverrideTrajectoryMultiplier;
		}

		if (Cost > LowestCost) 
		{
			continue; //Early Out
		}

		for (int32 i = 0; i < CurrentInterpolatedPose.JointData.Num(); ++i)
		{
			const FJointWeightSet WeightSet = FinalCalibration.PoseJointWeights[i];
			const FJointData CurrentJoint = CurrentInterpolatedPose.JointData[i];
			const FJointData CandidateJoint = Pose.JointData[i];

			Cost += FVector::DistSquared(CurrentJoint.Velocity, CandidateJoint.Velocity) * WeightSet.Weight_Vel * OverridePoseMultiplier;
			Cost += FVector::DistSquared(CurrentJoint.Position, CandidateJoint.Position) * WeightSet.Weight_Pos * OverridePoseMultiplier;
		}

		//Favour Current Pose
		if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
		{
			Cost *= CurrentPoseFavour;
		}

		//Apply Pose Favour
		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	const int32 DebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();
	if (DebugLevel == 1)
	{
		HistoricalPosesSearchCounts.Add(PoseCandidates->Num());
		HistoricalPosesSearchCounts.RemoveAt(0);

		DrawCandidateTrajectories(PoseCandidates);
	}
#endif

	return LowestPoseId;
}

int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId_Linear(const FPoseMotionData& NextPose)
{
	if (!FinalCalibrationSets.Contains(RequiredTraits))
	{
		return CurrentChosenPoseId;
	}

	const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

	const float OverridePoseMultiplier = (1.0f - OverrideQualityVsResponsivenessRatio) * 2.0f;
	const float OverrideTrajectoryMultiplier = OverrideQualityVsResponsivenessRatio * 2.0f;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : CurrentMotionData->Poses)
	{
		if (Pose.bDoNotUse || Pose.Traits != RequiredTraits)
		{
			continue;
		}

		//Body Velocity Cost
		float Cost = FVector::DistSquared(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity)
			* FinalCalibration.Weight_Momentum * OverridePoseMultiplier;

		//Body Rotational Velocity Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* FinalCalibration.Weight_AngularMomentum * OverridePoseMultiplier;

		if(Cost > LowestCost) 
		{
			continue; //Early out
		}

		//Pose Trajectory Cost
		Cost += FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
		                                                    Pose.Trajectory, FinalCalibration) * OverrideTrajectoryMultiplier;

		if(Cost > LowestCost) 
		{
			continue; //Early out
		}

		// Pose Joint Cost
		Cost += FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData,
			Pose.JointData, FinalCalibration) * OverridePoseMultiplier;
		
		//Pose Favour
		Cost *= Pose.Favour;

		//Favour Current Pose
		if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
		{
			Cost *= CurrentPoseFavour;
		}

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

	return LowestPoseId;
}

void FAnimNode_MSMotionMatching::TransitionToPose(const int32 PoseId, const FAnimationUpdateContext& Context, const float TimeOffset /*= 0.0f*/)
{
	switch (TransitionMethod)
	{
		case ETransitionMethod::None: { JumpToPose(PoseId, TimeOffset); } break;
		case ETransitionMethod::Blend: { BlendToPose(PoseId, TimeOffset); } break;
		case ETransitionMethod::Inertialization:
		{
			JumpToPose(PoseId, TimeOffset);
				
			UE::Anim::IInertializationRequester* InertializationRequester = Context.GetMessage<UE::Anim::IInertializationRequester>();
			if (InertializationRequester)
			{
				InertializationRequester->RequestInertialization(BlendTime);
				InertializationRequester->AddDebugRecord(*Context.AnimInstanceProxy, Context.GetCurrentNodeId());
			}
			else
			{
				//FAnimNode_Inertialization::LogRequestError(Context, BlendPose[ChildIndex]);
				UE_LOG(LogTemp, Error, TEXT("Motion Matching Node: Failed to get inertialisation node ancestor in the animation graph. Either add an inertialiation node or change the blend type."));
			}

		} break;
	}
}

void FAnimNode_MSMotionMatching::JumpToPose(const int32 PoseId, const float TimeOffset /*= 0.0f */)
{
	TimeSinceMotionChosen = TimeSinceMotionUpdate;
	CurrentChosenPoseId = PoseId;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	BlendChannels.Empty(TransitionMethod == ETransitionMethod::Blend ? 12 : 1);
	const FPoseMotionData& Pose = CurrentMotionData->Poses[PoseId];

	switch (Pose.AnimType)
	{
		//Sequence Pose
		case EMotionAnimAssetType::Sequence: 
		{
			const FMotionAnimSequence& MotionAnim = CurrentMotionData->GetSourceAnimAtIndex(Pose.AnimId);

			if (!MotionAnim.Sequence)
			{
				break;
			}

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionAnim.Sequence->GetPlayLength(), MotionAnim.bLoop, MotionAnim.PlayRate, Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));

		} break;
		//Blend Space Pose
		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = CurrentMotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

			if (!MotionBlendSpace.BlendSpace)
			{
				break;
			}

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionBlendSpace.GetPlayLength(), MotionBlendSpace.bLoop, MotionBlendSpace.PlayRate, Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));
				
			FAnimChannelState& blendChannel = BlendChannels.Last();	
				
			MotionBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(
				Pose.BlendSpacePosition.X, Pose.BlendSpacePosition.Y, 0.0f),
				blendChannel.BlendSampleDataCache, blendChannel.CachedTriangulationIndex, false);
				
		} break;
		//Composites
		case EMotionAnimAssetType::Composite:
		{
			const FMotionComposite& MotionComposite = CurrentMotionData->GetSourceCompositeAtIndex(Pose.AnimId);

			if (!MotionComposite.AnimComposite)
			{
				break;
			}

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionComposite.AnimComposite->GetPlayLength(), MotionComposite.bLoop, MotionComposite.PlayRate, Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));
		}
		default: 
		{ 
			return; 
		}
	}

	DominantBlendChannel = 0;
}

void FAnimNode_MSMotionMatching::BlendToPose(int32 PoseId, float TimeOffset /*= 0.0f */)
{
	TimeSinceMotionChosen = TimeSinceMotionUpdate;
	CurrentChosenPoseId = PoseId;

	//BlendChannels.Last().BlendStatus = EBlendStatus::Decay;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	const FPoseMotionData& Pose = CurrentMotionData->Poses[PoseId];

	switch (Pose.AnimType)
	{
		//Sequence Pose
		case EMotionAnimAssetType::Sequence:
		{
			const FMotionAnimSequence& MotionAnim = CurrentMotionData->GetSourceAnimAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Chosen, 1.0f,
				MotionAnim.Sequence->GetPlayLength(), MotionAnim.bLoop, MotionAnim.PlayRate,
				Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));

		} break;
		//Blend Space Pose
		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = CurrentMotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Chosen, 1.0f,
				MotionBlendSpace.GetPlayLength(), MotionBlendSpace.bLoop, MotionBlendSpace.PlayRate,
				Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));
				
			FAnimChannelState& blendChannel = BlendChannels.Last();
				
			MotionBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(
				Pose.BlendSpacePosition.X, Pose.BlendSpacePosition.Y, 0.0f),
				blendChannel.BlendSampleDataCache, blendChannel.CachedTriangulationIndex, false);
				
		} break;
		//Composites
		case EMotionAnimAssetType::Composite:
		{
			const FMotionComposite& MotionComposite = CurrentMotionData->GetSourceCompositeAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Chosen, 1.0f,
				MotionComposite.AnimComposite->GetPlayLength(), MotionComposite.bLoop, MotionComposite.PlayRate,
				Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));

		} break;
		//Default
		default: 
		{ 
			return; 
		}
	}
}

UMotionDataAsset* FAnimNode_MSMotionMatching::GetMotionData() const
{
	return GET_ANIM_NODE_DATA(TObjectPtr<UMotionDataAsset>, MotionData);
}

UMotionCalibration* FAnimNode_MSMotionMatching::GetUserCalibration() const
{
	return GET_ANIM_NODE_DATA(TObjectPtr<UMotionCalibration>, UserCalibration);
}

void FAnimNode_MSMotionMatching::CheckValidToEvaluate(const FAnimInstanceProxy* InAnimInstanceProxy)
{
	bValidToEvaluate = true;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	//Validate Motion Data
	if (!CurrentMotionData)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Motion Data has not been set."))
		bValidToEvaluate = false;
		return;
	}

	//Validate Motion Matching Configuration
	UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
	if (MMConfig)
	{
		MMConfig->Initialize();
		CurrentInterpolatedPose = FPoseMotionData(MMConfig->TrajectoryTimes.Num(), MMConfig->PoseBones.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Motion Match Config has not been set on the motion matching node."));
		bValidToEvaluate = false;
		return;
	}

	//Validate MMConfig matches internal calibration (i.e. the MMConfig has not been since changed
	for(auto& TraitCalibPair : CurrentMotionData->FeatureStandardDeviations)
	{
		FCalibrationData& CalibData = TraitCalibPair.Value;

		if (!CalibData.IsValidWithConfig(MMConfig))
		{
			UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Internal calibration sets atom count does not match the motion config. Did you change the motion config and forget to pre-process?"));
			bValidToEvaluate = false;
			return;
		}
	}

	//Validate Motion Matching optimization is setup correctly otherwise revert to Linear search
	if (PoseMatchMethod == EPoseMatchMethod::Optimized 
		&& CurrentMotionData->IsOptimisationValid())
	{
		CurrentMotionData->OptimisationModule->InitializeRuntime();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Motion matching node was set to run in optimized mode. However, the optimisation setup is invalid and optimization will be disabled. Did you forget to pre-process your motion data with optimisation on?"));
		PoseMatchMethod = EPoseMatchMethod::Linear;
	}

	UMotionCalibration* CurrentUserCalibration = GetUserCalibration();

	//If the user calibration is not set on the motion data asset, get it from the Motion Data instead
	if (!CurrentUserCalibration)
	{
		CurrentUserCalibration = CurrentMotionData->PreprocessCalibration;
	}

	if (CurrentUserCalibration)
	{
		CurrentUserCalibration->ValidateData();

		for (auto& FeatureStdDevPairs : CurrentMotionData->FeatureStandardDeviations)
		{
			FCalibrationData& NewFinalCalibration = FinalCalibrationSets.Add(FeatureStdDevPairs.Key, FCalibrationData());
			NewFinalCalibration.GenerateFinalWeights(CurrentUserCalibration, FeatureStdDevPairs.Value);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Motion Calibration not set in MotionData asset."));
		bValidToEvaluate = false;
		return;
	}

	JumpToPose(0);
	UAnimSequenceBase* Sequence = GetPrimaryAnim();
	const FAnimChannelState& PrimaryChannel = BlendChannels.Last();

	if (Sequence)
	{
		InternalTimeAccumulator = FMath::Clamp(PrimaryChannel.AnimTime, 0.0f, Sequence->GetPlayLength());

		if (PlaybackRate * Sequence->RateScale < 0.0f)
		{
			InternalTimeAccumulator = Sequence->GetPlayLength();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. The starting sequence is null. Check that all animations in the MotionData are valid"));
		bValidToEvaluate = false;
		return;
	}

	MirroringData.Initialize(CurrentMotionData->MirroringProfile, InAnimInstanceProxy->GetSkelMeshComponent());
}

bool FAnimNode_MSMotionMatching::NextPoseToleranceTest(FPoseMotionData& NextPose)
{
	if (NextPose.bDoNotUse 
	|| NextPose.Traits != RequiredTraits)
	{
		return false;
	}

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

	//We already know that the next Pose data will have good Pose transition so we only
	//need to test trajectory (closeness). Additionally there is no need to test past trajectory
	const int32 PointCount = FMath::Min(DesiredTrajectory.TrajectoryPoints.Num(), MMConfig->TrajectoryTimes.Num());
	for (int32 i = 0; i < PointCount; ++i)
	{
		const float PredictionTime = MMConfig->TrajectoryTimes[i];

		if (PredictionTime > 0.0f)
		{
			const float RelativeTolerance_Pos = PredictionTime * PositionTolerance;
			const float RelativeTolerance_Angle = PredictionTime * RotationTolerance;

			FTrajectoryPoint& NextPoint = NextPose.Trajectory[i];
			FTrajectoryPoint& DesiredPoint = DesiredTrajectory.TrajectoryPoints[i];

			FVector DiffVector = NextPoint.Position - DesiredPoint.Position;
			const float SqrDistance = DiffVector.SizeSquared();
			
			if (SqrDistance > RelativeTolerance_Pos * RelativeTolerance_Pos)
			{
				return false;
			}

			const float AngleDelta = FMath::FindDeltaAngleDegrees(DesiredPoint.RotationZ, NextPoint.RotationZ);
		
			if (FMath::Abs(AngleDelta) > RelativeTolerance_Angle)
			{
				return false;
			}
		}
	}

	return true;
}

void FAnimNode_MSMotionMatching::ApplyTrajectoryBlending()
{
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
	if(!MMConfig)
	{
		return;
	}
	
	const float TotalTime = FMath::Max(0.0001f, MMConfig->TrajectoryTimes.Last());

	const int32 TrajectoryIterations = FMath::Min(DesiredTrajectory.TrajectoryPoints.Num(),
		MMConfig->TrajectoryTimes.Num());
	
	for (int32 i = 0; i < TrajectoryIterations; ++i)
	{
		const float Time = MMConfig->TrajectoryTimes[i];
		if (Time > 0.0f)
		{
			FTrajectoryPoint& DesiredPoint = DesiredTrajectory.TrajectoryPoints[i];
			FTrajectoryPoint& CurrentPoint = CurrentInterpolatedPose.Trajectory[i];

			float Progress = ((TotalTime - Time) / TotalTime) * TrajectoryBlendMagnitude;

			DesiredPoint.Position = FMath::Lerp(DesiredPoint.Position, CurrentPoint.Position, Progress);
		}
	}
}

float FAnimNode_MSMotionMatching::GetCurrentAssetTime() const
{
	return InternalTimeAccumulator;
}


float FAnimNode_MSMotionMatching::GetCurrentAssetTimePlayRateAdjusted() const
{
	UAnimSequenceBase* Sequence = GetPrimaryAnim();

	const float EffectivePlayRate = PlaybackRate * (Sequence ? Sequence->RateScale : 1.0f);
	const float Length = Sequence ? Sequence->GetPlayLength() : 0.0f;

	return (EffectivePlayRate < 0.0f) ? Length - InternalTimeAccumulator : InternalTimeAccumulator;
}

float FAnimNode_MSMotionMatching::GetCurrentAssetLength() const
{
	UAnimSequenceBase* Sequence = GetPrimaryAnim();
	return Sequence ? Sequence->GetPlayLength() : 0.0f;
}

UAnimationAsset* FAnimNode_MSMotionMatching::GetAnimAsset() const
{
	return GetMotionData();
}

bool FAnimNode_MSMotionMatching::NeedsOnInitializeAnimInstance() const
{
	return true;
}

void FAnimNode_MSMotionMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InAnimInstanceProxy, const UAnimInstance* InAnimInstance)
{
	Super::OnInitializeAnimInstance(InAnimInstanceProxy, InAnimInstance);

	CheckValidToEvaluate(InAnimInstanceProxy);
}

void FAnimNode_MSMotionMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
	
	GetEvaluateGraphExposedInputs().Execute(Context);

	if(!bValidToEvaluate)
	{
		CheckValidToEvaluate(Context.AnimInstanceProxy);
	}
	
	UMotionDataAsset* CurrentMotionData = GetMotionData();

	InternalTimeAccumulator = 0.0f;
	
	if (!CurrentMotionData 
	|| !CurrentMotionData->bIsProcessed
	|| !bValidToEvaluate)
	{
		return;
	}
		
	MotionMatchingMode = EMotionMatchingMode::MotionMatching;

	if(bInitialized)
	{
		bTriggerTransition = true;
	}

	AnimInstanceProxy = Context.AnimInstanceProxy;
}

void FAnimNode_MSMotionMatching::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateAssetPlayer)
	
	GetEvaluateGraphExposedInputs().Execute(Context);

	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	if (!CurrentMotionData 
	|| !CurrentMotionData->bIsProcessed
	|| !bValidToEvaluate)
	{
		UE_LOG(LogTemp, Error, TEXT("Motion Matching node failed to update properly as the setup is not valid."))
		return;
	}

	const float DeltaTime = Context.GetDeltaTime();

	if (!bInitialized)
	{
		InitializeWithPoseRecorder(Context);
		bInitialized = true;
	}

	//Check for triggered action
	if (MotionActionPayload.ActionId > -1) 
	{
		InitializeMotionAction(Context);
	}

	//Update based on motion matching node
	switch (MotionMatchingMode)
	{
		case EMotionMatchingMode::MotionMatching:
		{
			UpdateMotionMatchingState(DeltaTime, Context);
		} break;
		case EMotionMatchingMode::DistanceMatching:
		{
			UpdateDistanceMatchingState(DeltaTime, Context);
		} break;
		case EMotionMatchingMode::Action:
		{
			UpdateMotionActionState(DeltaTime, Context);
		} break;
	}

	const FAnimChannelState& CurrentChannel = BlendChannels.Last();

	CreateTickRecordForNode(Context, PlaybackRate * CurrentChannel.PlayRate);

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	//Visualize the motion matching search / optimisation debugging
	const int32 SearchDebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();
	if (SearchDebugLevel == 1)
	{
		DrawSearchCounts(Context.AnimInstanceProxy);
	}

	//Visualize the trajectroy debugging
	const int32 TrajDebugLevel = CVarMMTrajectoryDebug.GetValueOnAnyThread();
	
	if (TrajDebugLevel > 0)
	{
		if (TrajDebugLevel == 2)
		{
			//Draw chosen trajectory
			DrawChosenTrajectoryDebug(Context.AnimInstanceProxy);
		}

		//Draw Input trajectory
		DrawTrajectoryDebug(Context.AnimInstanceProxy);
	}

	const int32 PoseDebugLevel = CVarMMPoseDebug.GetValueOnAnyThread();

	if (PoseDebugLevel > 0)
	{
		DrawChosenPoseDebug(Context.AnimInstanceProxy, PoseDebugLevel > 1);
	}

	//Debug the current animation data being played by the motion matching node
	const int32 AnimDebugLevel = CVarMMAnimDebug.GetValueOnAnyThread();

	if(AnimDebugLevel > 0)
	{
		DrawAnimDebug(Context.AnimInstanceProxy);
	}
#endif
}

void FAnimNode_MSMotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Evaluate_AnyThread)
	
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	if (!CurrentMotionData 
	|| !CurrentMotionData->bIsProcessed
	|| !IsLODEnabled(Output.AnimInstanceProxy))
	{
		Output.Pose.ResetToRefPose();
		return;
	}

	const int32 ChannelCount = BlendChannels.Num();

	if (ChannelCount == 0)
	{
		Output.Pose.ResetToRefPose();
		return;
	}
	
	if (ChannelCount > 1 && BlendTime > 0.00001f)
	{
		EvaluateBlendPose(Output);
	}
	else
	{
		EvaluateSinglePose(Output);
	}
}

void FAnimNode_MSMotionMatching::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	
	const FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);

	//Todo: Add more debug information here!
}

void FAnimNode_MSMotionMatching::EvaluateSinglePose(FPoseContext& Output)
{
	FAnimChannelState& PrimaryChannel = BlendChannels.Last();
	float AnimTime = PrimaryChannel.AnimTime;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	switch (PrimaryChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence:
		{
			const FMotionAnimSequence& MotionSequence = CurrentMotionData->GetSourceAnimAtIndex(PrimaryChannel.AnimId);
			const UAnimSequence* AnimSequence = MotionSequence.Sequence;
				
			if(MotionSequence.bLoop)
			{
				AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, AnimSequence->GetPlayLength());
			}
				
			FAnimationPoseData AnimationPoseData(Output);
			AnimSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(AnimTime, true));
		} break;

		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = CurrentMotionData->GetSourceBlendSpaceAtIndex(PrimaryChannel.AnimId);
			const UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;

			if (!BlendSpace)
			{
				break;
			}

			if (MotionBlendSpace.bLoop)
			{
				AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, MotionBlendSpace.GetPlayLength());
			}

			for (int32 i = 0; i < PrimaryChannel.BlendSampleDataCache.Num(); ++i)
			{
				PrimaryChannel.BlendSampleDataCache[i].Time = AnimTime;
			}
				
			FAnimationPoseData AnimationPoseData(Output);
			BlendSpace->GetAnimationPose(PrimaryChannel.BlendSampleDataCache, FAnimExtractContext(AnimTime,
				Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, PrimaryChannel.bLoop), AnimationPoseData);
		} break;

		case EMotionAnimAssetType::Composite:
		{
			const FMotionComposite& MotionComposite = CurrentMotionData->GetSourceCompositeAtIndex(PrimaryChannel.AnimId);
			const UAnimComposite* Composite = MotionComposite.AnimComposite;

			if(!Composite)
			{
				break;
			}

			if(MotionComposite.bLoop)
			{
				AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, Composite->GetPlayLength());
			}
				
			FAnimationPoseData AnimationPoseData(Output);
			Composite->GetAnimationPose(AnimationPoseData, FAnimExtractContext(PrimaryChannel.AnimTime, true));
		} break;
		default: ;
	}

	if (PrimaryChannel.bMirrored)
	{
		FMotionMatchingUtils::MirrorPose(Output.Pose, CurrentMotionData->MirroringProfile, MirroringData, 
			Output.AnimInstanceProxy->GetSkelMeshComponent());
	}
}

void FAnimNode_MSMotionMatching::EvaluateBlendPose(FPoseContext& Output)
{
	
	
	const int32 PoseCount = BlendChannels.Num();

	if (PoseCount > 0)
	{
		//Prepare containers for blending

		TArray<FCompactPose, TInlineAllocator<8>> ChannelPoses;
		ChannelPoses.AddZeroed(PoseCount);

		TArray<FBlendedCurve, TInlineAllocator<8>> ChannelCurves;
		ChannelCurves.AddZeroed(PoseCount);
		
		TArray<UE::Anim::FStackAttributeContainer, TInlineAllocator<8>> ChannelAttributes;
		ChannelAttributes.AddZeroed(PoseCount);

		TArray<float, TInlineAllocator<8>> ChannelWeights;
		ChannelWeights.AddZeroed(PoseCount);

		TArray<FTransform> ChannelRootMotions;
		ChannelRootMotions.AddZeroed(PoseCount);

		const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();

		for (int32 i = 0; i < ChannelPoses.Num(); ++i)
		{
			ChannelPoses[i].SetBoneContainer(&BoneContainer);
			ChannelCurves[i].InitFrom(Output.Curve);
		}

		//Extract poses from each channel

		UMotionDataAsset* CurrentMotionData = GetMotionData();

		float TotalBlendPower = 0.0f;
		for (int32 i = 0; i < PoseCount; ++i)
		{
			FCompactPose& Pose = ChannelPoses[i];
			FAnimChannelState& AnimChannel = BlendChannels[i];

			const float Weight = AnimChannel.Weight * ((((float)(i + 1)) / ((float)PoseCount)));
			ChannelWeights[i] = Weight;
			TotalBlendPower += Weight;

			float AnimTime = AnimChannel.AnimTime;

			switch (AnimChannel.AnimType)
			{
				case EMotionAnimAssetType::Sequence:
				{
					const FMotionAnimSequence& MotionAnim = CurrentMotionData->GetSourceAnimAtIndex(AnimChannel.AnimId);
					const UAnimSequence* AnimSequence = MotionAnim.Sequence;

					if(!AnimSequence)
					{
						break;
					}

					if (MotionAnim.bLoop)
					{
						AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, AnimSequence->GetPlayLength());
					}
						
					FAnimationPoseData AnimationPoseData = { Pose, ChannelCurves[i], ChannelAttributes[i] };
					AnimSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(AnimTime, true));
				} break;
				case EMotionAnimAssetType::BlendSpace:
				{
					const FMotionBlendSpace& MotionBlendSpace = CurrentMotionData->GetSourceBlendSpaceAtIndex(AnimChannel.AnimId);
					const UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;

					if(!BlendSpace)
					{
						break;
					}

					if (MotionBlendSpace.bLoop)
					{
						AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, MotionBlendSpace.GetPlayLength());
					}

					for (int32 k = 0; k < AnimChannel.BlendSampleDataCache.Num(); ++k)
					{
						AnimChannel.BlendSampleDataCache[k].Time = AnimTime;
					}
						
					FAnimationPoseData AnimationPoseData = { Pose, ChannelCurves[i], ChannelAttributes[i] };
					BlendSpace->GetAnimationPose(AnimChannel.BlendSampleDataCache, FAnimExtractContext(AnimTime,
					Output.AnimInstanceProxy->ShouldExtractRootMotion(), DeltaTimeRecord, AnimChannel.bLoop), AnimationPoseData);
				}
				break;
				default: ;
			}

			if(AnimChannel.bMirrored)
			{
				FMotionMatchingUtils::MirrorPose(Pose, CurrentMotionData->MirroringProfile, MirroringData, Output.AnimInstanceProxy->GetSkelMeshComponent());
			}
		}

		//Blend poses together according to their weights
		if (TotalBlendPower > 0.0f)
		{
			for (int32 i = 0; i < PoseCount; ++i)
			{
				ChannelWeights[i] = ChannelWeights[i] / TotalBlendPower;
			}

			const TArrayView<const FCompactPose> ChannelPoseView(ChannelPoses);
			

			const TArrayView<const FBlendedCurve> ChannelCurveView(ChannelCurves);
			const TArrayView<const UE::Anim::FStackAttributeContainer> ChannelAttributeView(ChannelAttributes);
			const TArrayView<const float> ChannelWeightsView(ChannelWeights);
			FAnimationPoseData AnimationPoseData(Output);
			FAnimationRuntime::BlendPosesTogether(ChannelPoseView, ChannelCurveView,  ChannelAttributeView, ChannelWeightsView, AnimationPoseData);
			Output.Pose.NormalizeRotations();
		}
		else
		{
			if(const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim())
			{
				FAnimationPoseData AnimationPoseData(Output);
				PrimaryAnim->GetAnimationPose(AnimationPoseData, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
			}
		}
	}
	else
	{
		if (const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim())
		{
			FAnimationPoseData AnimationPoseData(Output);
			PrimaryAnim->GetAnimationPose(AnimationPoseData, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
		}
	}
}


void FAnimNode_MSMotionMatching::CreateTickRecordForNode(const FAnimationUpdateContext& Context, const float PlayRate)
{
	// Create a tick record and fill it out
	const float FinalBlendWeight = Context.GetFinalBlendWeight();
	
	UE::Anim::FAnimSyncGroupScope& SyncScope = Context.GetMessageChecked<UE::Anim::FAnimSyncGroupScope>();
	
	const FName GroupNameToUse = ((GetGroupRole() < EAnimGroupRole::TransitionLeader) || bHasBeenFullWeight) ? GetGroupName() : NAME_None;
	EAnimSyncMethod MethodToUse = GetGroupMethod();
	if(GroupNameToUse == NAME_None && MethodToUse == EAnimSyncMethod::SyncGroup)
	{
		MethodToUse = EAnimSyncMethod::DoNotSync;
	}

	const UE::Anim::FAnimSyncParams SyncParams(GroupNameToUse, GetGroupRole(), MethodToUse);
	FAnimTickRecord TickRecord(nullptr, true, PlayRate, FinalBlendWeight, /*inout*/ InternalTimeAccumulator, MarkerTickRecord);
    
	TickRecord.SourceAsset = GetMotionData();
	TickRecord.TimeAccumulator = &InternalTimeAccumulator;
	TickRecord.MarkerTickRecord = &MarkerTickRecord;
	TickRecord.PlayRateMultiplier = PlayRate;
	TickRecord.EffectiveBlendWeight = FinalBlendWeight;
	TickRecord.bLooping = true;
	TickRecord.bCanUseMarkerSync = false;
	TickRecord.BlendSpace.BlendSpacePositionX = 0.0f;
	TickRecord.BlendSpace.BlendSpacePositionY = 0.0f;
	TickRecord.BlendSpace.BlendFilter = nullptr;
	TickRecord.BlendSpace.BlendSampleDataCache = reinterpret_cast<TArray<FBlendSampleData>*>(&BlendChannels);
	TickRecord.RootMotionWeightModifier = Context.GetRootMotionWeightModifier();
	
	SyncScope.AddTickRecord(TickRecord, SyncParams, UE::Anim::FAnimSyncDebugInfo(Context));
    
	TRACE_ANIM_TICK_RECORD(Context, TickRecord);
}

void FAnimNode_MSMotionMatching::PerformLinearSearchComparison(const FAnimationUpdateContext& Context, int32 ComparePoseId, FPoseMotionData& NextPose)
{
	int32 LowestPoseId = LowestPoseId = GetLowestCostPoseId_Linear(NextPose);

	const bool SamePoseChosen = LowestPoseId == ComparePoseId;

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	LowestPoseId = FMath::Clamp(LowestPoseId, 0, CurrentMotionData->Poses.Num() - 1);

	float LinearChosenPoseCost = 0.0f;
	const float ActualChosenPoseCost = 0.0f;

	float LinearChosenTrajectoryCost = 0.0f;
	float ActualChosenTrajectoryCost = 0.0f;
	
	if (!SamePoseChosen)
	{
		const FPoseMotionData& LinearPose = CurrentMotionData->Poses[LowestPoseId];
		const FPoseMotionData& ActualPose = CurrentMotionData->Poses[ComparePoseId];

		LinearChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, LinearPose.Trajectory,
			1.0f, 0.0f);

		ActualChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, ActualPose.Trajectory,
			1.0f, 0.0f);

		LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData, LinearPose.JointData,
			1.0f, 0.0f);

		LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData, ActualPose.JointData,
			1.0f, 0.0f);
	}

	const UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

	const float TrajectorySearchError = FMath::Abs(ActualChosenTrajectoryCost - LinearChosenTrajectoryCost) / MMConfig->TrajectoryTimes.Num();
	const float PoseSearchError = FMath::Abs(ActualChosenPoseCost - LinearChosenPoseCost) / MMConfig->PoseBones.Num();

	const FString OverallMessage = FString::Printf(TEXT("Linear Search Error %f"), PoseSearchError + TrajectorySearchError);
	const FString PoseMessage = FString::Printf(TEXT("Linear Search Pose Error %f"), PoseSearchError);
	const FString TrajectoryMessage = FString::Printf(TEXT("Linear Search Trajectory Error %f"), TrajectorySearchError);
	Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(OverallMessage, FColor::Black);
	Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(PoseMessage, FColor::Red);
	Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(TrajectoryMessage, FColor::Blue);
}

UAnimSequence* FAnimNode_MSMotionMatching::GetAnimAtIndex(const int32 AnimId)
{
	if(AnimId < 0 
	|| AnimId >= BlendChannels.Num())
	{
		return nullptr;
	}

	const FAnimChannelState& AnimChannel = BlendChannels[AnimId];

	return GetMotionData()->GetSourceAnimAtIndex(AnimChannel.AnimId).Sequence;
}

UAnimSequenceBase* FAnimNode_MSMotionMatching::GetPrimaryAnim()
{
	if (BlendChannels.Num() == 0)
	{
		return nullptr;
	}

	const FAnimChannelState& CurrentChannel = BlendChannels.Last();

	switch (CurrentChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: return GetMotionData()->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
		case EMotionAnimAssetType::Composite: return GetMotionData()->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
		default: return nullptr;
	}
}

UAnimSequenceBase* FAnimNode_MSMotionMatching::GetPrimaryAnim() const
{
	if (BlendChannels.Num() == 0)
	{
		return nullptr;
	}

	const FAnimChannelState& CurrentChannel = BlendChannels.Last();

	switch (CurrentChannel.AnimType)
	{
		case EMotionAnimAssetType::Sequence: return GetMotionData()->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
		case EMotionAnimAssetType::Composite: return GetMotionData()->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
		default: return nullptr;
	}
}

void FAnimNode_MSMotionMatching::DrawTrajectoryDebug(FAnimInstanceProxy* InAnimInstanceProxy)
{
	if(!InAnimInstanceProxy
		|| DesiredTrajectory.TrajectoryPoints.Num() == 0)
	{
		return;
	}

	UMotionMatchConfig* MMConfig = GetMotionData()->MotionMatchConfig;
	
	const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector ActorLocation = MeshTransform.GetLocation();
	FVector LastPoint;

	const float FacingOffset = FMotionMatchingUtils::GetFacingAngleOffset(MMConfig->ForwardAxis);

	for (int32 i = 0; i < DesiredTrajectory.TrajectoryPoints.Num(); ++i)
	{
		FColor Color = FColor::Green;
		if (MMConfig->TrajectoryTimes[i] < 0.0f)
		{
			Color = FColor(0, 128, 0);
		}

		FTrajectoryPoint& TrajPoint = DesiredTrajectory.TrajectoryPoints[i];

		FVector PointPosition = MeshTransform.TransformPosition(TrajPoint.Position);
		InAnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);

		FQuat ArrowRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(TrajPoint.RotationZ + FacingOffset));
		FVector DrawTo = PointPosition + (ArrowRotation * MeshTransform.TransformVector(FVector::ForwardVector) * 30.0f);

		InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);

		if(i > 0)
		{
			if (MMConfig->TrajectoryTimes[i - 1 ] < 0.0f && MMConfig->TrajectoryTimes[i] > 0.0f)
			{
				InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, ActorLocation, FColor::Blue, false, -1.0f, 2.0f);
				InAnimInstanceProxy->AnimDrawDebugLine(ActorLocation, PointPosition, FColor::Blue, false, -1.0f, 2.0f);
				InAnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Blue, false, -1.0f);
			}
			else
			{
				InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, PointPosition, Color, false, -1.0f, 2.0f);
			}
		}

		LastPoint = PointPosition;
	}
}

void FAnimNode_MSMotionMatching::DrawChosenTrajectoryDebug(FAnimInstanceProxy* InAnimInstanceProxy)
{
	UMotionDataAsset* CurrentMotionData = GetMotionData();
	
	if (InAnimInstanceProxy == nullptr 
	|| CurrentChosenPoseId > CurrentMotionData->Poses.Num() - 1)
	{
		return;
	}

	TArray<FTrajectoryPoint>& CurrentTrajectory = CurrentMotionData->Poses[CurrentChosenPoseId].Trajectory;

	if (CurrentTrajectory.Num() == 0)
	{
		return;
	}

	UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
	
	const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	const FVector ActorLocation = MeshTransform.GetLocation();
	FVector LastPoint;

	const float FacingOffset = FMotionMatchingUtils::GetFacingAngleOffset(MMConfig->ForwardAxis);

	for (int32 i = 0; i < CurrentTrajectory.Num(); ++i)
	{
		FColor Color = FColor::Red;
		if (MMConfig->TrajectoryTimes[i] < 0.0f)
		{
			Color = FColor(128, 0, 0);
		}

		FTrajectoryPoint& TrajPoint = CurrentTrajectory[i];

		FVector PointPosition = MeshTransform.TransformPosition(TrajPoint.Position);
		InAnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);

		FQuat ArrowRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(TrajPoint.RotationZ + FacingOffset));
		FVector DrawTo = PointPosition + (ArrowRotation * MeshTransform.TransformVector(FVector::ForwardVector) * 30.0f);

		InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);

		if (i > 0)
		{
			if (MMConfig->TrajectoryTimes[i - 1] < 0.0f && MMConfig->TrajectoryTimes[i] > 0.0f)
			{
				InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, ActorLocation, FColor::Orange, false, -1.0f, 2.0f);
				InAnimInstanceProxy->AnimDrawDebugLine(ActorLocation, PointPosition, FColor::Orange, false, -1.0f, 2.0f);
				InAnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Orange, false, -1.0f);
			}
			else
			{
				InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, PointPosition, Color, false, -1.0f, 2.0f);
			}
		}

		LastPoint = PointPosition;
	}
}

void FAnimNode_MSMotionMatching::DrawChosenPoseDebug(FAnimInstanceProxy* InAnimInstanceProxy, bool bDrawVelocity)
{
	if (InAnimInstanceProxy == nullptr)
	{
		return;
	}

	FPoseMotionData& ChosenPose = CurrentInterpolatedPose;
	TArray<FJointData>& PoseJoints = ChosenPose.JointData;

	if (PoseJoints.Num() == 0)
	{
		return;
	}

	const FTransform& ActorTransform = InAnimInstanceProxy->GetActorTransform();
	const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	FVector ActorLocation = MeshTransform.GetLocation();

	//Draw Body Velocity
	InAnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Blue, false, -1.0f, 0.0f);
	InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(ActorLocation, MeshTransform.TransformPosition(
		ChosenPose.LocalVelocity), 80.0f, FColor::Blue, false, -1.0f, 3.0f);

	for (int32 i = 0; i < PoseJoints.Num(); ++i)
	{
		FJointData& Joint = PoseJoints[i];

		FColor Color = FColor::Yellow;

		FVector JointPosition = MeshTransform.TransformPosition(Joint.Position);
		InAnimInstanceProxy->AnimDrawDebugSphere(JointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);

		if (bDrawVelocity)
		{
			FVector DrawTo = MeshTransform.TransformPosition(Joint.Position + (Joint.Velocity * 0.33333f));
			InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(JointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);
		}
	}
}

void FAnimNode_MSMotionMatching::DrawCandidateTrajectories(TArray<FPoseMotionData>* PoseCandidates)
{
	if (!AnimInstanceProxy
	|| !PoseCandidates)
	{
		return;
	}

	FTransform CharTransform = AnimInstanceProxy->GetActorTransform();
	CharTransform.ConcatenateRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, -90.0f)));

	for (FPoseMotionData& Candidate : *PoseCandidates)
	{
		DrawPoseTrajectory(AnimInstanceProxy, Candidate, CharTransform);
	}
}

void FAnimNode_MSMotionMatching::DrawPoseTrajectory(FAnimInstanceProxy* InAnimInstanceProxy, FPoseMotionData& Pose, FTransform& CharTransform)
{
	if (!InAnimInstanceProxy)
	{
		return;
	}

	FVector LastPoint = CharTransform.TransformPosition(Pose.Trajectory[0].Position);
	LastPoint.Z -= 87.0f;

	for (int32 i = 1; i < Pose.Trajectory.Num(); ++i)
	{
		FVector ThisPoint = CharTransform.TransformPosition(Pose.Trajectory[i].Position);
		ThisPoint.Z -= 87.0f;
		InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, ThisPoint, FColor::Orange, false, 0.1f, 1.0f);
		LastPoint = ThisPoint;
	}
}

void FAnimNode_MSMotionMatching::DrawSearchCounts(FAnimInstanceProxy* InAnimInstanceProxy)
{
	if (!InAnimInstanceProxy)
	{
		return;
	}

	int32 MaxCount = -1;
	int32 MinCount = 100000000;
	int32 AveCount = 0;
	const int32 LatestCount = HistoricalPosesSearchCounts.Last();
	for (int32 Count : HistoricalPosesSearchCounts)
	{
		AveCount += Count;

		if (Count > MaxCount)
		{
			MaxCount = Count;
		}

		if (Count < MinCount)
		{
			MinCount = Count;
		}
	}

	AveCount /= HistoricalPosesSearchCounts.Num();

	const int32 PoseCount = GetMotionData()->Poses.Num();

	const FString TotalMessage = FString::Printf(TEXT("Total Poses: %02d"), PoseCount);
	const FString LastMessage = FString::Printf(TEXT("Poses Searched: %02d (%f % Reduction)"), LatestCount, ((float)PoseCount - (float)LatestCount) / (float)PoseCount * 100.0f);
	const FString AveMessage = FString::Printf(TEXT("Average: %02d (%f % Reduction)"), AveCount, ((float)PoseCount - (float)AveCount) / (float)PoseCount * 100.0f);
	const FString MaxMessage = FString::Printf(TEXT("High: %02d (%f % Reduction)"), MaxCount, ((float)PoseCount - (float)MaxCount) / (float)PoseCount * 100.0f);
	const FString MinMessage = FString::Printf(TEXT("Low: %02d (%f % Reduction)\n"), MinCount, ((float)PoseCount - (float)MinCount) / (float)PoseCount * 100.0f);
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(TotalMessage, FColor::Black);
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(LastMessage, FColor::Purple);
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(AveMessage, FColor::Blue);
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(MaxMessage, FColor::Red);
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(MinMessage, FColor::Green);
}

void FAnimNode_MSMotionMatching::DrawAnimDebug(FAnimInstanceProxy* InAnimInstanceProxy) const
{
	if(!InAnimInstanceProxy)
	{
		return;
	}

	UMotionDataAsset* CurrentMotionData = GetMotionData();

	FPoseMotionData& CurrentPose = CurrentMotionData->Poses[FMath::Clamp(CurrentInterpolatedPose.PoseId,
			0, CurrentMotionData->Poses.Num())];

	//Print Pose Information
	FString Message = FString::Printf(TEXT("Pose Id: %02d \nPoseFavour: %f \nMirrored: "),
		CurrentPose.PoseId, CurrentPose.Favour);
	
	if(CurrentPose.bMirrored)
	{
		Message += FString(TEXT("True\n"));
	}
	else
	{
		Message += FString(TEXT("False\n"));
	}
	
	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(Message, FColor::Green);
	
	//Print Animation Information
	FString AnimMessage = FString::Printf(TEXT("Anim Id: %02d \nAnimType: "), CurrentPose.AnimId);

	switch(CurrentPose.AnimType)
	{
		case EMotionAnimAssetType::Sequence: AnimMessage += FString(TEXT("Sequence \n")); break;
		case EMotionAnimAssetType::BlendSpace: AnimMessage += FString(TEXT("Blend Space \n")); break;
		case EMotionAnimAssetType::Composite: AnimMessage += FString(TEXT("Composite \n")); break;
		default: AnimMessage += FString(TEXT("Invalid \n")); break;
	}
	const FAnimChannelState& AnimChannel = BlendChannels.Last();
	AnimMessage += FString::Printf(TEXT("Anim Time: %0f \nAnimName: "), AnimChannel.AnimTime);
	
	FMotionAnimAsset* MotionAnimAsset = CurrentMotionData->GetSourceAnim(CurrentPose.AnimId, CurrentPose.AnimType);
	if(MotionAnimAsset && MotionAnimAsset->AnimAsset)
	{
		AnimMessage += MotionAnimAsset->AnimAsset->GetName();
	}
	else
	{
		AnimMessage += FString::Printf(TEXT("Invalid \n"));
	}

	InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(AnimMessage, FColor::Blue);

	//Last Chosen Pose
	//Last Pose Cost
}