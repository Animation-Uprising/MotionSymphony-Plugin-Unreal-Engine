// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_MotionMatching.h"
#include "MotionSymphony.h"
#include "MotionMatchingUtil/MotionMatchingUtils.h"
#include "AnimationRuntime.h"
#include "Enumerations/EMotionMatchingEnums.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Controller.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_Inertialization.h"

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

FAnimNode_MotionMatching::FAnimNode_MotionMatching() :
	UpdateInterval(0.1f),
	PlaybackRate(1.0f),
	BlendTime(0.3f),
	MotionData(nullptr),
	bBlendOutEarly(true),
	PoseMatchMethod(EPoseMatchMethod::HighQuality),
	TransitionMethod(ETransitionMethod::Blend),
	PastTrajectoryMode(EPastTrajectoryMode::ActualHistory),
	bBlendTrajectory(false),
	TrajectoryBlendMagnitude(1.0f),
	bFavourCurrentPose(false),
	CurrentPoseFavour(0.95f),
	bEnableToleranceTest(true),
	PositionTolerance(50.0f),
	RotationTolerance(2.0f),
	TimeSinceMotionUpdate(0.0f),
	TimeSinceMotionChosen(0.0f),
	PoseInterpolationValue(0.0f),
	bForcePoseSearch(false),
	CurrentChosenPoseId(0),
	DominantBlendChannel(0),
	bInitialized(false),
	bTriggerTransition(false)
{
	DesiredTrajectory.Clear();
	BlendChannels.Empty(12);
	HistoricalPosesSearchCounts.Empty(11);
	
	for (int32 i = 0; i < 30; ++i)
	{
		HistoricalPosesSearchCounts.Add(0);
	}
}

FAnimNode_MotionMatching::~FAnimNode_MotionMatching()
{

}

void FAnimNode_MotionMatching::UpdateBlending(const float DeltaTime)
{
	float HighestBlendWeight = -1.0f;
	int32 HighestBlendChannel = 0;
	for (int32 i = 0; i < BlendChannels.Num(); ++i)
	{
		bool Current = i == BlendChannels.Num() - 1;

		float Weight = BlendChannels[i].Update(DeltaTime, BlendTime, Current);

		if (Weight < -0.5f)
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

void FAnimNode_MotionMatching::InitializeWithPoseRecorder(const FAnimationUpdateContext& Context)
{
	FAnimNode_MotionRecorder* MotionRecorderNode = Context.GetAncestor<FAnimNode_MotionRecorder>();
	MotionRecorderNode->RegisterBoneIdsToRecord(MotionData->PoseJoints);
}

void FAnimNode_MotionMatching::InitializeMatchedTransition(const FAnimationUpdateContext& Context)
{
	TimeSinceMotionChosen = 0.0f;
	TimeSinceMotionUpdate = 0.0f;

	FAnimNode_MotionRecorder* MotionRecorderNode = Context.GetAncestor<FAnimNode_MotionRecorder>();

	if (MotionRecorderNode)
	{
		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
		ScheduleTransitionPoseSearch(Context);
	}
	else
	{
		//We just jump tot he default pose because there is no way to match to external nodes.
		JumpToPose(0);
		return;
	}
}

void FAnimNode_MotionMatching::UpdateMotionMatching(const float DeltaTime, const FAnimationUpdateContext& Context)
{
	//Todo: Detect idle and trigger if valid
	bForcePoseSearch = false;
	TimeSinceMotionChosen += DeltaTime;
	TimeSinceMotionUpdate += DeltaTime;

	FAnimChannelState& PrimaryChannel = BlendChannels.Last();
	int32 AnimId = PrimaryChannel.AnimId;

	if (!PrimaryChannel.bLoop)
	{
		float CurrentBlendTime = 0.0f;

		if (bBlendOutEarly)
			CurrentBlendTime = BlendTime * PrimaryChannel.Weight * PlaybackRate;

		if (TimeSinceMotionChosen + PrimaryChannel.StartTime + CurrentBlendTime
			> PrimaryChannel.AnimLength)
		{
			bForcePoseSearch = true;
		}
	}

	FAnimNode_MotionRecorder* MotionRecorderNode = Context.GetAncestor<FAnimNode_MotionRecorder>();

	if (MotionRecorderNode)
	{
		ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
	}
	else
	{
		ComputeCurrentPose();
	}

	//Past trajectory mode
	if (PastTrajectoryMode == EPastTrajectoryMode::CopyFromCurrentPose)
	{
		for (int32 i = 0; i < MotionData->TrajectoryTimes.Num(); ++i)
		{
			if (MotionData->TrajectoryTimes[i] > 0.0f)
				break;

			DesiredTrajectory.TrajectoryPoints[i] = CurrentInterpolatedPose.Trajectory[i];
		}
	}

	if (TimeSinceMotionUpdate > UpdateInterval)
	{
		TimeSinceMotionUpdate -= UpdateInterval;
		SchedulePoseSearch(DeltaTime, Context);
	}
	else if(bForcePoseSearch)
	{
		TimeSinceMotionUpdate = 0.0f;
		SchedulePoseSearch(DeltaTime, Context);
	}
}

void FAnimNode_MotionMatching::ComputeCurrentPose()
{
	const float PoseInterval = MotionData->PoseInterval;

	//====== Determine the next chosen pose ========
	FAnimChannelState& ChosenChannel = BlendChannels.Last();

	const float ChosenClipLength = MotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).Sequence->SequenceLength;

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
			float TimeToNextClip = ChosenClipLength - (TimePassed + ChosenChannel.StartTime);

			if (TimeToNextClip < PoseInterval / 2.0f)
				--PoseIndex;

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
	FAnimChannelState& DominantChannel = BlendChannels[DominantBlendChannel];

	const float DominantClipLength = MotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).Sequence->SequenceLength;

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
			float TimeToNextClip = DominantClipLength - (TimePassed + DominantChannel.StartTime);

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

	PoseIndex = FMath::Clamp(PoseIndex + NumPosesPassed, 0, MotionData->Poses.Num());

	//Get the before and after poses and then interpolate
	FPoseMotionData* BeforePose;
	FPoseMotionData* AfterPose;

	if (TimePassed < -0.00001f)
	{
		AfterPose = &MotionData->Poses[PoseIndex];
		BeforePose = &MotionData->Poses[FMath::Clamp(AfterPose->LastPoseId, 0, MotionData->Poses.Num() - 1)];

		PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - (float)NumPosesPassed);
	}
	else
	{
		BeforePose = &MotionData->Poses[FMath::Min(PoseIndex, MotionData->Poses.Num() - 2)];
		AfterPose = &MotionData->Poses[BeforePose->NextPoseId];

		PoseInterpolationValue = (TimePassed / PoseInterval) - (float)NumPosesPassed;
	}

	FMotionMatchingUtils::LerpPose(CurrentInterpolatedPose, *BeforePose, *AfterPose, PoseInterpolationValue);
}

void FAnimNode_MotionMatching::ComputeCurrentPose(const FCachedMotionPose& CachedMotionPose)
{
	const float PoseInterval = MotionData->PoseInterval;

	//====== Determine the next chosen pose ========
	FAnimChannelState& ChosenChannel = BlendChannels.Last();

	const float ChosenClipLength = MotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).Sequence->SequenceLength;

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
			float TimeToNextClip = ChosenClipLength - (TimePassed + ChosenChannel.StartTime);

			if (TimeToNextClip < PoseInterval / 2.0f)
				--PoseIndex;

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
	FAnimChannelState& DominantChannel = BlendChannels[DominantBlendChannel];

	const float DominantClipLength = MotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).Sequence->SequenceLength;

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
			float TimeToNextClip = DominantClipLength - (TimePassed + DominantChannel.StartTime);

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

	PoseIndex = FMath::Clamp(PoseIndex + NumPosesPassed, 0, MotionData->Poses.Num());

	//Get the before and after poses and then interpolate
	FPoseMotionData* BeforePose;
	FPoseMotionData* AfterPose;

	if (TimePassed < -0.00001f)
	{
		AfterPose = &MotionData->Poses[PoseIndex];
		BeforePose = &MotionData->Poses[FMath::Clamp(AfterPose->LastPoseId, 0, MotionData->Poses.Num() - 1)];

		PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - (float)NumPosesPassed);
	}
	else
	{
		BeforePose = &MotionData->Poses[FMath::Min(PoseIndex, MotionData->Poses.Num() - 2)];
		AfterPose = &MotionData->Poses[BeforePose->NextPoseId];

		PoseInterpolationValue = (TimePassed / PoseInterval) - (float)NumPosesPassed;
	}

	FMotionMatchingUtils::LerpPoseTrajectory(CurrentInterpolatedPose, *BeforePose, *AfterPose, PoseInterpolationValue);

	for (int32 i = 0; i < MotionData->PoseJoints.Num(); ++i)
	{
		const FCachedMotionBone& CachedMotionBone = CachedMotionPose.CachedBoneData[MotionData->PoseJoints[i]];

		CurrentInterpolatedPose.JointData[i] = FJointData(CachedMotionBone.Transform.GetLocation(), CachedMotionBone.Velocity);
	}
}

void FAnimNode_MotionMatching::SchedulePoseSearch(float DeltaTime, const FAnimationUpdateContext& Context)
{
	if (bBlendTrajectory)
		ApplyTrajectoryBlending();

	//TODO: Out of range exception here sometimes WTF!??
	if(CurrentChosenPoseId >= MotionData->Poses.Num())
		CurrentChosenPoseId = MotionData->Poses.Num() - 1; //Just in case

	FPoseMotionData& NextPose = MotionData->Poses[MotionData->Poses[CurrentChosenPoseId].NextPoseId];

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
		case EPoseMatchMethod::LowQuality: { LowestPoseId = GetLowestCostPoseId_LQ(NextPose); } break;
		case EPoseMatchMethod::HighQuality: { LowestPoseId = GetLowestCostPoseId_HQ(NextPose); } break;
		case EPoseMatchMethod::LowQuality_Linear: { LowestPoseId = GetLowestCostPoseId_LQ_Linear(NextPose); } break;
		case EPoseMatchMethod::HighQuality_Linear: { LowestPoseId = GetLowestCostPoseId_HQ_Linear(NextPose); } break;
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	int32 DebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();

	if(DebugLevel == 2)
		PerformLinearSearchComparrison(LowestPoseId, NextPose);
#endif

	FPoseMotionData& BestPose = MotionData->Poses[LowestPoseId];
	FPoseMotionData& ChosenPose = MotionData->Poses[CurrentChosenPoseId];

	bool bWinnerAtSameLocation = BestPose.AnimId == CurrentInterpolatedPose.AnimId &&
								 BestPose.bMirrored == CurrentInterpolatedPose.bMirrored &&
								FMath::Abs(BestPose.Time - CurrentInterpolatedPose.Time) < 0.25f;

	if (!bWinnerAtSameLocation)
	{
		bWinnerAtSameLocation = BestPose.AnimId == ChosenPose.AnimId &&
								BestPose.bMirrored == ChosenPose.bMirrored &&
								FMath::Abs(BestPose.Time - ChosenPose.Time) < 0.25f;
	}

	if (!bWinnerAtSameLocation)
	{
		TransitionToPose(BestPose.PoseId, Context);
	}
}

void FAnimNode_MotionMatching::ScheduleTransitionPoseSearch(const FAnimationUpdateContext & Context)
{
	int32 LowestPoseId = -1;
	switch (PoseMatchMethod)
	{
		case EPoseMatchMethod::LowQuality: { LowestPoseId = GetLowestCostPoseId_LQ(); } break;
		case EPoseMatchMethod::HighQuality: { LowestPoseId = GetLowestCostPoseId_HQ(); } break;
	}

	LowestPoseId = FMath::Clamp(LowestPoseId, 0, MotionData->Poses.Num() - 1);
	JumpToPose(LowestPoseId);

}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_LQ()
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : MotionData->Poses)
	{
		if (Pose.bDoNotUse)
			continue;

		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		Cost += FMotionMatchingUtils::ComputePoseCost_SD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity);

		//Todo: Re-Add this once the motion recorder records character velocity
		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

	return LowestPoseId;
}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_LQ(FPoseMotionData& NextPose)
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	FPoseCandidateSet CandidateSet = MotionData->PoseLookupTable.CandidateSets[CurrentInterpolatedPose.CandidateSetId];
	for (int32 PoseId : CandidateSet.PoseCandidates)
	{
		FPoseMotionData& Pose = MotionData->Poses[PoseId];

		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		Cost += FMotionMatchingUtils::ComputePoseCost_SD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity);

		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
			Cost *= CurrentPoseFavour;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	int32 DebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();
	if (DebugLevel == 1)
	{
		HistoricalPosesSearchCounts.Add(CandidateSet.PoseCandidates.Num());
		HistoricalPosesSearchCounts.RemoveAt(0);

		DrawCandidateTrajectories(CandidateSet);
	}
#endif

	return LowestPoseId;
}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_HQ()
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : MotionData->Poses)
	{
		if (Pose.bDoNotUse)
			continue;

		//Pose Trajectory Cost
		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		//Pose Joint Cost
		Cost += FMotionMatchingUtils::ComputePoseCost_HD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity,
			Calibration.PoseWeight_ResVelocity, MotionData->PoseInterval);

		//Body Velocity Cost
		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		//Body Rotational Velocity Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* Calibration.BodyWeight_RotationalVelocity;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

	return LowestPoseId;
}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_HQ(FPoseMotionData& NextPose)
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	FPoseCandidateSet& CandidateSet = MotionData->PoseLookupTable.CandidateSets[CurrentInterpolatedPose.CandidateSetId];
	for (int32 PoseId : CandidateSet.PoseCandidates)
	{
		FPoseMotionData& Pose = MotionData->Poses[PoseId];

		//Pose Trajectory Cost
		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		//Pose Joint Cost
		Cost += FMotionMatchingUtils::ComputePoseCost_HD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity,
			Calibration.PoseWeight_ResVelocity, MotionData->PoseInterval);

		//Body Velocity Cost
		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		//Body Rotational Velocity Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* Calibration.BodyWeight_RotationalVelocity;

		if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
		{
			Cost *= CurrentPoseFavour;
		}

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	int32 DebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();
	if (DebugLevel == 1)
	{
		HistoricalPosesSearchCounts.Add(CandidateSet.PoseCandidates.Num());
		HistoricalPosesSearchCounts.RemoveAt(0);

		DrawCandidateTrajectories(CandidateSet);
	}
#endif

	return LowestPoseId;
}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_LQ_Linear(FPoseMotionData& NextPose)
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : MotionData->Poses)
	{
		if (Pose.bDoNotUse)
			continue;

		//Pose Trajectory Cost
		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		// Pose Joint Cost
		Cost += FMotionMatchingUtils::ComputePoseCost_SD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity);

		//Body Velocity Cost
		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		//Body Rotational Velocity Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* Calibration.BodyWeight_RotationalVelocity;

		if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
			Cost *= CurrentPoseFavour;

		Cost *= Pose.Favour;

		if (Cost < LowestCost)
		{
			LowestCost = Cost;
			LowestPoseId = Pose.PoseId;
		}
	}

	return LowestPoseId;
}

int32 FAnimNode_MotionMatching::GetLowestCostPoseId_HQ_Linear(FPoseMotionData& NextPose)
{
	int32 LowestPoseId = 0;
	float LowestCost = 10000000.0f;
	for (FPoseMotionData& Pose : MotionData->Poses)
	{
		if (Pose.bDoNotUse)
			continue;

		//Pose Trajectory Cost
		float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			Pose.Trajectory, Calibration.TrajectoryWeight_Position, Calibration.TrajectoryWeight_Rotation);

		//Pose Joint Cost
		Cost += FMotionMatchingUtils::ComputePoseCost_HD(CurrentInterpolatedPose.JointData,
			Pose.JointData, Calibration.PoseWeight_Position, Calibration.PoseWeight_Velocity,
			Calibration.PoseWeight_ResVelocity, MotionData->PoseInterval);

		//Body Velocity Cost
		Cost += FVector::Distance(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity) * Calibration.BodyWeight_Velocity;

		//Body Rotational Velocity Cost
		Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			* Calibration.BodyWeight_RotationalVelocity;

if (bFavourCurrentPose && Pose.PoseId == NextPose.PoseId)
{
	Cost *= CurrentPoseFavour;
}

Cost *= Pose.Favour;

if (Cost < LowestCost)
{
	LowestCost = Cost;
	LowestPoseId = Pose.PoseId;
}
	}

	return LowestPoseId;
}

void FAnimNode_MotionMatching::TransitionToPose(int32 PoseId, const FAnimationUpdateContext& Context)
{
	switch (TransitionMethod)
	{
	case ETransitionMethod::None: { JumpToPose(PoseId); } break;
	case ETransitionMethod::Blend: { BlendToPose(PoseId); } break;
	case ETransitionMethod::Inertialization:
	{
		JumpToPose(PoseId);

		FAnimNode_Inertialization* InertializationNode = Context.GetAncestor<FAnimNode_Inertialization>();
		if (InertializationNode)
		{
			InertializationNode->RequestInertialization(BlendTime);
		}
		else
		{
			//Todo: Log request error
			//FAnimNode_Inertialization::LogRequestError(Context, )
		}

	} break;
	}
}

void FAnimNode_MotionMatching::JumpToPose(int32 PoseId)
{
	TimeSinceMotionChosen = TimeSinceMotionUpdate;
	CurrentChosenPoseId = PoseId;

	BlendChannels.Empty(TransitionMethod == ETransitionMethod::Blend ? 12 : 1);
	FPoseMotionData& Pose = MotionData->Poses[PoseId];

	switch (Pose.AnimType)
	{
		//Sequence Pose
		case EMotionAnimAssetType::Sequence: 
		{
			const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionAnim.Sequence->SequenceLength, MotionAnim.bLoop, Pose.bMirrored, TimeSinceMotionChosen));
		} break;
		//Blend Space Pose
		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionBlendSpace.BlendSpace->AnimLength, MotionBlendSpace.bLoop, Pose.bMirrored, TimeSinceMotionChosen));

			MotionBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(
				Pose.BlendSpacePosition.X, Pose.BlendSpacePosition.Y, 0.0f), 
				BlendChannels.Last().BlendSampleDataCache);

		} break;
		default: { return; }
	}

	DominantBlendChannel = 0;
}

void FAnimNode_MotionMatching::BlendToPose(int32 PoseId)
{
	TimeSinceMotionChosen = TimeSinceMotionUpdate;
	CurrentChosenPoseId = PoseId;

	BlendChannels.Last().BlendStatus = EBlendStatus::Decay;

	FPoseMotionData& Pose = MotionData->Poses[PoseId];

	switch (Pose.AnimType)
	{
		//Sequence Pose
		case EMotionAnimAssetType::Sequence:
		{
			const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionAnim.Sequence->SequenceLength, MotionAnim.bLoop, Pose.bMirrored, TimeSinceMotionChosen));

		} break;
		//Blend Space Pose
		case EMotionAnimAssetType::BlendSpace:
		{
			const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

			BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Dominant, 1.0f,
				MotionBlendSpace.BlendSpace->AnimLength, MotionBlendSpace.bLoop, Pose.bMirrored, TimeSinceMotionChosen));

			MotionBlendSpace.BlendSpace->GetSamplesFromBlendInput(FVector(
				Pose.BlendSpacePosition.X, Pose.BlendSpacePosition.Y, 0.0f),
				BlendChannels.Last().BlendSampleDataCache);

		} break;
		default: { return; }
	}
}

bool FAnimNode_MotionMatching::NextPoseToleranceTest(FPoseMotionData& NextPose)
{
	if (NextPose.bDoNotUse)
		return false;

	//We already know that the next Pose data will have good Pose transition so we only
	//need to test trajectory (closeness). Additionally there is no need to test past trajectory
	int32 PointCount = DesiredTrajectory.TrajectoryPoints.Num();
	for (int32 i = 0; i < PointCount; ++i)
	{
		float PredictionTime = MotionData->TrajectoryTimes[i];

		if (PredictionTime > 0.0f)
		{
			float RelativeTolerance_Pos = PredictionTime * PositionTolerance;
			float RelativeTolerance_Angle = PredictionTime * RotationTolerance;

			FTrajectoryPoint& NextPoint = NextPose.Trajectory[i];
			FTrajectoryPoint& DesiredPoint = DesiredTrajectory.TrajectoryPoints[i];

			FVector DiffVector = NextPoint.Position - DesiredPoint.Position;
			float SqrDistance = DiffVector.SizeSquared();
			
			if (SqrDistance > RelativeTolerance_Pos * RelativeTolerance_Pos)
				return false;

			float AngleDelta = FMath::FindDeltaAngleDegrees(DesiredPoint.RotationZ - 90.0f, NextPoint.RotationZ);
		
			if (FMath::Abs(AngleDelta) > RelativeTolerance_Angle)
				return false;
		}
	}

	return true;
}

void FAnimNode_MotionMatching::ApplyTrajectoryBlending()
{
	float TotalTime = MotionData->TrajectoryTimes.Last();

	for (int i = 0; i < MotionData->TrajectoryTimes.Num(); ++i)
	{
		float Time = MotionData->TrajectoryTimes[i];
		if (Time > 0.0f)
		{
			FTrajectoryPoint& DesiredPoint = DesiredTrajectory.TrajectoryPoints[i];
			FTrajectoryPoint& CurrentPoint = CurrentInterpolatedPose.Trajectory[i];

			float Progress = ((TotalTime - Time) / TotalTime) * TrajectoryBlendMagnitude;

			DesiredPoint.Position = FMath::Lerp(DesiredPoint.Position, CurrentPoint.Position, Progress);
		}
	}
}

float FAnimNode_MotionMatching::GetCurrentAssetTime()
{
	return InternalTimeAccumulator;
}

float FAnimNode_MotionMatching::GetCurrentAssetTimePlayRateAdjusted()
{
	UAnimSequence* Sequence = GetPrimaryAnim();

	float EffectivePlayrate = PlaybackRate * (Sequence ? Sequence->RateScale : 1.0f);

	return (EffectivePlayrate < 0.0f) ? GetCurrentAssetLength() - InternalTimeAccumulator : InternalTimeAccumulator;
}

float FAnimNode_MotionMatching::GetCurrentAssetLength()
{
	UAnimSequence* Sequence = GetPrimaryAnim();
	return Sequence ? Sequence->SequenceLength : 0.0f;
}

UAnimationAsset * FAnimNode_MotionMatching::GetAnimAsset()
{
	return MotionData;
}

void FAnimNode_MotionMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	GetEvaluateGraphExposedInputs().Execute(Context);

	InternalTimeAccumulator = 0.0f;

	if (!MotionData || !MotionData->bIsProcessed)
		return;

	if(!bInitialized)
	{
		if (!MotionData->bIsOptimised)
		{
			switch (PoseMatchMethod)
			{
				case EPoseMatchMethod::LowQuality: { PoseMatchMethod = EPoseMatchMethod::LowQuality_Linear; } break;
				case EPoseMatchMethod::HighQuality: { PoseMatchMethod = EPoseMatchMethod::HighQuality_Linear; } break;
			}
		}

		CurrentInterpolatedPose = FPoseMotionData(MotionData->TrajectoryTimes.Num(), MotionData->PoseJoints.Num());
		JumpToPose(0);

		UAnimSequence* Sequence = GetPrimaryAnim();

		FAnimChannelState& primaryState = BlendChannels.Last();

		if (Sequence)
		{
			InternalTimeAccumulator = FMath::Clamp(primaryState.AnimTime, 0.0f, Sequence->SequenceLength);

			if (PlaybackRate * Sequence->RateScale < 0.0f)
			{
				InternalTimeAccumulator = Sequence->SequenceLength;
			}
		}

		MirroringData.Initialize(MotionData->MirroringProfile, Context.AnimInstanceProxy->GetSkelMeshComponent());
	}
	else
	{
		bTriggerTransition = true;
	}

	AnimInstanceProxy = Context.AnimInstanceProxy;
}

void FAnimNode_MotionMatching::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) 
{
}

void FAnimNode_MotionMatching::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	GetEvaluateGraphExposedInputs().Execute(Context);

	if (!MotionData || !MotionData->bIsProcessed)
		return;

	float DeltaTime = Context.GetDeltaTime();

	if (!bInitialized)
	{
		InitializeWithPoseRecorder(Context);
		bInitialized = true;
	}

	if (bTriggerTransition)
	{
		InitializeMatchedTransition(Context);
		bTriggerTransition = false;
	}
	else
	{
		UpdateMotionMatching(DeltaTime, Context);
		UpdateBlending(DeltaTime);
	}

	

	UAnimSequence* CurrentAnim = GetPrimaryAnim();

	if (CurrentAnim != nullptr && Context.AnimInstanceProxy->IsSkeletonCompatible(CurrentAnim->GetSkeleton()))
	{
		InternalTimeAccumulator = FMath::Clamp(InternalTimeAccumulator, 0.0f, CurrentAnim->SequenceLength);
		CreateTickRecordForNode(Context, CurrentAnim, true, PlaybackRate);
	}

#if ENABLE_ANIM_DEBUG && ENABLE_DRAW_DEBUG
	int32 SearchDebugLevel = CVarMMSearchDebug.GetValueOnAnyThread();

	if (SearchDebugLevel == 1)
		DrawSearchCounts();

	int32 TrajDebugLevel = CVarMMTrajectoryDebug.GetValueOnAnyThread();
	
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

	int32 PoseDebugLevel = CVarMMPoseDebug.GetValueOnAnyThread();

	if (PoseDebugLevel > 0)
	{
		DrawChosenPoseDebug(Context.AnimInstanceProxy, PoseDebugLevel > 1);
	}

#endif
}

void FAnimNode_MotionMatching::Evaluate_AnyThread(FPoseContext& Output)
{
	if (!MotionData || !MotionData->bIsProcessed)
		return;

	const int32 ChannelCount = BlendChannels.Num();

	if (ChannelCount == 0)
	{
		Output.Pose.ResetToRefPose();
		return;
	}
	
	if (ChannelCount > 1 && BlendTime > 0.00001f)
	{
		EvaluateBlendPose(Output, Output.AnimInstanceProxy->GetDeltaSeconds());
	}
	else
	{
		FAnimChannelState& PrimaryChannel = BlendChannels.Last();
		float AnimTime = PrimaryChannel.AnimTime;

		switch (PrimaryChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence:
			{
				const FMotionAnimSequence& MotionSequence = MotionData->GetSourceAnimAtIndex(PrimaryChannel.AnimId);

				if(MotionSequence.bLoop)
					AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, MotionSequence.Sequence->SequenceLength);

#if ENGINE_MINOR_VERSION > 25
				FAnimationPoseData AnimationPoseData(Output);
				GetPrimaryAnim()->GetAnimationPose(AnimationPoseData, FAnimExtractContext(PrimaryChannel.AnimTime, true));
#else
				GetPrimaryAnim()->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(PrimaryChannel.AnimTime, true));
#endif
			} break;

			case EMotionAnimAssetType::BlendSpace:
			{
				const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(PrimaryChannel.AnimId);
				UBlendSpaceBase* BlendSpace = MotionBlendSpace.BlendSpace;

				if (!BlendSpace)
					break;

				if (MotionBlendSpace.bLoop)
					AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, BlendSpace->AnimLength);

#if ENGINE_MINOR_VERSION > 25
				FAnimationPoseData AnimationPoseData(Output);
				BlendSpace->GetAnimationPose(PrimaryChannel.BlendSampleDataCache, AnimationPoseData);
#else
				BlendSpace->GetAnimationPose(PrimaryChannel.BlendSampleDataCache, Output.Pose, Output.Curve);
#endif
			} break;
		}

		if (PrimaryChannel.bMirrored)
			FMotionMatchingUtils::MirrorPose(Output.Pose, MotionData->MirroringProfile, MirroringData, Output.AnimInstanceProxy->GetSkelMeshComponent());
	}
}

void FAnimNode_MotionMatching::OverrideAsset(UAnimationAsset* NewAsset)
{
}

void FAnimNode_MotionMatching::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
}

bool FAnimNode_MotionMatching::HasPreUpdate() const
{
	return true;
}

void FAnimNode_MotionMatching::PreUpdate(const UAnimInstance* InAnimInstance)
{
	//TODO: Determine the current pose

	//ComputeCurrentPose();
}

void FAnimNode_MotionMatching::EvaluateBlendPose(FPoseContext& Output, const float DeltaTime)
{
	const int32 PoseCount = BlendChannels.Num();

	if (PoseCount > 0)
	{
		//Prepare containers for blending

		TArray<FCompactPose, TInlineAllocator<8>> ChannelPoses;
		ChannelPoses.AddZeroed(PoseCount);

		TArray<FBlendedCurve, TInlineAllocator<8>> ChannelCurves;
		ChannelCurves.AddZeroed(PoseCount);

#if ENGINE_MINOR_VERSION > 25
		TArray<FStackCustomAttributes, TInlineAllocator<8>> ChannelAttributes;
		ChannelAttributes.AddZeroed(PoseCount);
#endif

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

		float TotalBlendPower = 0.0f;
		for (int32 i = 0; i < PoseCount; ++i)
		{
			FCompactPose& Pose = ChannelPoses[i];
			FAnimChannelState& AnimChannel = BlendChannels[i];

			float Weight = AnimChannel.Weight * ((((float)(i + 1)) / ((float)PoseCount)));
			ChannelWeights[i] = Weight;
			TotalBlendPower += Weight;

			float AnimTime = AnimChannel.AnimTime;

			switch (AnimChannel.AnimType)
			{
				case EMotionAnimAssetType::Sequence:
				{
					const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(AnimChannel.AnimId);
					const UAnimSequence* AnimSequence = MotionAnim.Sequence;

					if (MotionAnim.bLoop)
						AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, AnimSequence->SequenceLength);

#if ENGINE_MINOR_VERSION > 25
					FAnimationPoseData AnimationPoseData = { Pose, ChannelCurves[i], ChannelAttributes[i] };
					MotionAnim.Sequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(AnimTime, true));
#else
					MotionAnim.Sequence->GetAnimationPose(Pose, ChannelCurves[i], FAnimExtractContext(AnimTime, true));
#endif
				} break;
				case EMotionAnimAssetType::BlendSpace:
				{
					const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(AnimChannel.AnimId);
					UBlendSpaceBase* BlendSpace = MotionBlendSpace.BlendSpace;

					if(!BlendSpace)
						break;

					if (MotionBlendSpace.bLoop)
						AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, BlendSpace->AnimLength);

#if ENGINE_MINOR_VERSION > 25
					FAnimationPoseData AnimationPoseData = { Pose, ChannelCurves[i], ChannelAttributes[i] };
					BlendSpace->GetAnimationPose(AnimChannel.BlendSampleDataCache, AnimationPoseData);
#else
					BlendSpace->GetAnimationPose(AnimChannel.BlendSampleDataCache, Pose, ChannelCurves);
#endif
				}
				break;
			}

			if(AnimChannel.bMirrored && AnimInstanceProxy)
				FMotionMatchingUtils::MirrorPose(Pose, MotionData->MirroringProfile, MirroringData, AnimInstanceProxy->GetSkelMeshComponent());
		}

		//Blend poses together according to their weights
		if (TotalBlendPower > 0.0f)
		{
			//Todo: Re-visit this method of calculating blends
			for (int32 i = 0; i < PoseCount; ++i)
			{
				ChannelWeights[i] = ChannelWeights[i] / TotalBlendPower;
			}

			TArrayView<FCompactPose> ChannelPoseView(ChannelPoses);
	
#if ENGINE_MINOR_VERSION > 25
			FAnimationPoseData AnimationPoseData(Output);
			FAnimationRuntime::BlendPosesTogether(ChannelPoseView, ChannelCurves, ChannelAttributes, ChannelWeights, AnimationPoseData);
#else
			FAnimationRuntime::BlendPosesTogether(ChannelPoseView, ChannelCurves, ChannelWeights, Output.Pose, Output.Curve);
				
#endif

			Output.Pose.NormalizeRotations();
		}
		else
		{
#if ENGINE_MINOR_VERSION > 25
			FAnimationPoseData AnimationPoseData(Output);
			GetPrimaryAnim()->GetAnimationPose(AnimationPoseData, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
#else 
			GetPrimaryAnim()->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
#endif
		}
	}
	else
	{
#if ENGINE_MINOR_VERSION > 25
		FAnimationPoseData AnimationPoseData(Output);
		GetPrimaryAnim()->GetAnimationPose(AnimationPoseData, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
#else
		GetPrimaryAnim()->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
#endif
	}
}


void FAnimNode_MotionMatching::CreateTickRecordForNode(const FAnimationUpdateContext& Context,
	UAnimSequenceBase* Sequence, bool bLooping, float PlayRate)
{
	// Create a tick record and fill it out
	const float FinalBlendWeight = Context.GetFinalBlendWeight();

	FAnimGroupInstance* SyncGroup;

#if ENGINE_MINOR_VERSION > 25
	const FName GroupNameToUse = ((GroupRole < EAnimGroupRole::TransitionLeader) || bHasBeenFullWeight) ? GroupName : NAME_None;
	FAnimTickRecord& TickRecord = Context.AnimInstanceProxy->CreateUninitializedTickRecordInScope(/*out*/ SyncGroup, GroupNameToUse, GroupScope);
#else
	const int32 GroupIndexToUse = ((GroupRole < EAnimGroupRole::TransitionLeader) || bHasBeenFullWeight) ? GroupIndex : INDEX_NONE;
	FAnimTickRecord& TickRecord = Context.AnimInstanceProxy->CreateUninitializedTickRecord(GroupIndexToUse, /*out*/ SyncGroup);
#endif

	TickRecord.SourceAsset = MotionData;
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

	// Update the sync group if it exists
	if (SyncGroup != NULL)
	{
		SyncGroup->TestTickRecordForLeadership(GroupRole);
	}

	TRACE_ANIM_TICK_RECORD(Context, TickRecord);
}

void FAnimNode_MotionMatching::PerformLinearSearchComparrison(int32 ComparePoseId, FPoseMotionData& NextPose)
{
	int32 LowestPoseId = -1;
	switch (PoseMatchMethod)
	{
		case EPoseMatchMethod::LowQuality:
		{
			LowestPoseId = GetLowestCostPoseId_LQ_Linear(NextPose);

		} break;
		case EPoseMatchMethod::HighQuality:
		{
			LowestPoseId = GetLowestCostPoseId_HQ_Linear(NextPose);

		} break;
	}

	bool SamePoseChosen = LowestPoseId == ComparePoseId;

	float LinearChosenPoseCost = 0.0f;
	float ActualChosenPoseCost = 0.0f;

	float LinearChosenTrajectoryCost = 0.0f;
	float ActualChosenTrajectoryCost = 0.0f;
	
	if (!SamePoseChosen)
	{
		FPoseMotionData& LinearPose = MotionData->Poses[LowestPoseId];
		FPoseMotionData& ActualPose = MotionData->Poses[ComparePoseId];

		LinearChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, LinearPose.Trajectory,
			1.0f, 0.0f);

		ActualChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, ActualPose.Trajectory,
			1.0f, 0.0f);

		LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost_SD(CurrentInterpolatedPose.JointData, LinearPose.JointData,
			1.0f, 0.0f);

		LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost_SD(CurrentInterpolatedPose.JointData, ActualPose.JointData,
			1.0f, 0.0f);
	}

	float TrajectorySearchError = FMath::Abs(ActualChosenTrajectoryCost - LinearChosenPoseCost) / MotionData->TrajectoryTimes.Num();
	float PoseSearchError = FMath::Abs(ActualChosenPoseCost - LinearChosenPoseCost) / MotionData->PoseJoints.Num();;

	const FString OverallMessage = FString::Printf(TEXT("Linear Search Error %f"), PoseSearchError + TrajectorySearchError);
	const FString PoseMessage = FString::Printf(TEXT("Linear Search Pose Error %f"), PoseSearchError);
	const FString TrajectoryMessage = FString::Printf(TEXT("Linear Search Trajectory Error %f"), TrajectorySearchError);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(OverallMessage, FColor::Black);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(PoseMessage, FColor::Red);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(TrajectoryMessage, FColor::Blue);
}

UAnimSequence* FAnimNode_MotionMatching::GetAnimAtIndex(const int32 AnimId)
{
	if(AnimId < 0 || AnimId >= BlendChannels.Num())
		return nullptr;

	FAnimChannelState& AnimChannel = BlendChannels[AnimId];

	return MotionData->GetSourceAnimAtIndex(AnimChannel.AnimId).Sequence;
}

UAnimSequence* FAnimNode_MotionMatching::GetPrimaryAnim()
{
	if (BlendChannels.Num() == 0)
		return nullptr;

	FAnimChannelState & CurrentChannel = BlendChannels.Last();

	return MotionData->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
}

void FAnimNode_MotionMatching::DrawTrajectoryDebug(FAnimInstanceProxy* InAnimInstanceProxy)
{
	if(InAnimInstanceProxy == nullptr)
		return;

	if(DesiredTrajectory.TrajectoryPoints.Num() == 0)
		return;

	const FTransform& ActorTransform = InAnimInstanceProxy->GetActorTransform();
	const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	FVector ActorLocation = MeshTransform.GetLocation();
	FVector LastPoint;

	for (int32 i = 0; i < DesiredTrajectory.TrajectoryPoints.Num(); ++i)
	{
		FColor Color = FColor::Green;
		if (MotionData->TrajectoryTimes[i] < 0.0f)
		{
			Color = FColor(0, 128, 0);
		}

		FTrajectoryPoint& TrajPoint = DesiredTrajectory.TrajectoryPoints[i];

		FVector PointPosition = MeshTransform.TransformPosition(TrajPoint.Position);
		InAnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);

		FQuat ArrowRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(TrajPoint.RotationZ));
		FVector DrawTo = PointPosition + (ArrowRotation * ActorTransform.TransformVector(FVector::ForwardVector) * 30.0f);

		InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);

		/*const FString TrajPointMsg = FString::Printf(TEXT("Trajectory Point %02d: %f degrees)"), i, TrajPoint.RotationZ);
		InAnimInstanceProxy->AnimDrawDebugOnScreenMessage(TrajPointMsg, FColor::Green);*/

		if(i > 0)
		{
			if (MotionData->TrajectoryTimes[i - 1 ] < 0.0f && MotionData->TrajectoryTimes[i] > 0.0f)
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

void FAnimNode_MotionMatching::DrawChosenTrajectoryDebug(FAnimInstanceProxy* InAnimInstanceProxy)
{
	if (InAnimInstanceProxy == nullptr)
		return;

	TArray<FTrajectoryPoint>& CurrentTrajectory = MotionData->Poses[CurrentChosenPoseId].Trajectory;

	if (CurrentTrajectory.Num() == 0)
		return;

	const FTransform& ActorTransform = InAnimInstanceProxy->GetActorTransform();
	const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
	FVector ActorLocation = MeshTransform.GetLocation();
	FVector LastPoint;

	for (int32 i = 0; i < CurrentTrajectory.Num(); ++i)
	{
		FColor Color = FColor::Red;
		if (MotionData->TrajectoryTimes[i] < 0.0f)
		{
			Color = FColor(128, 0, 0);
		}

		FTrajectoryPoint& TrajPoint = CurrentTrajectory[i];

		FVector PointPosition = MeshTransform.TransformPosition(TrajPoint.Position);
		InAnimInstanceProxy->AnimDrawDebugSphere(PointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);

		FQuat ArrowRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(TrajPoint.RotationZ));
		FVector DrawTo = PointPosition + (ArrowRotation * ActorTransform.TransformVector(FVector::ForwardVector) * 30.0f);

		InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(PointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);

		if (i > 0)
		{
			if (MotionData->TrajectoryTimes[i - 1] < 0.0f && MotionData->TrajectoryTimes[i] > 0.0f)
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

void FAnimNode_MotionMatching::DrawChosenPoseDebug(FAnimInstanceProxy* InAnimInstanceProxy, bool bDrawVelocity)
{
	if (InAnimInstanceProxy == nullptr)
		return;

	FPoseMotionData& ChosenPose = CurrentInterpolatedPose;
	TArray<FJointData>& PoseJoints = ChosenPose.JointData;

	if (PoseJoints.Num() == 0)
		return;

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

void FAnimNode_MotionMatching::DrawCandidateTrajectories(FPoseCandidateSet& Candidates)
{
	if (!AnimInstanceProxy)
		return;

	FTransform CharTransform = AnimInstanceProxy->GetActorTransform();
	CharTransform.ConcatenateRotation(FQuat::MakeFromEuler(FVector(0.0f, 0.0f, -90.0f)));

	for (int32 CandidateId : Candidates.PoseCandidates)
	{
		DrawPoseTrajectory(MotionData->Poses[CandidateId], CharTransform);
	}
}

void FAnimNode_MotionMatching::DrawPoseTrajectory(FPoseMotionData& Pose, FTransform& CharTransform)
{

	FVector LastPoint = CharTransform.TransformPosition(Pose.Trajectory[0].Position);
	LastPoint.Z -= 87.0f;

	for (int32 i = 1; i < Pose.Trajectory.Num(); ++i)
	{
		FVector ThisPoint = CharTransform.TransformPosition(Pose.Trajectory[i].Position);
		ThisPoint.Z -= 87.0f;
		AnimInstanceProxy->AnimDrawDebugLine(LastPoint, ThisPoint, FColor::Orange, false, 0.1f, 1.0f);
		LastPoint = ThisPoint;
	}
}

void FAnimNode_MotionMatching::DrawSearchCounts()
{
	if (!AnimInstanceProxy)
		return;

	int32 MaxCount = -1;
	int32 MinCount = 100000000;
	int32 AveCount = 0;
	int32 LatestCount = HistoricalPosesSearchCounts.Last();
	for (int32 Count : HistoricalPosesSearchCounts)
	{
		AveCount += Count;

		if (Count > MaxCount)
			MaxCount = Count;

		if (Count < MinCount)
			MinCount = Count;
	}

	AveCount /= HistoricalPosesSearchCounts.Num();

	int32 PoseCount = MotionData->Poses.Num();

	const FString TotalMessage = FString::Printf(TEXT("Total Poses: %02d"), MotionData->Poses.Num());
	const FString LastMessage = FString::Printf(TEXT("Poses Searched: %02d (%f % Reduction)"), LatestCount, ((float)PoseCount - (float)LatestCount) / (float)PoseCount * 100.0f);
	const FString AveMessage = FString::Printf(TEXT("Average: %02d (%f % Reduction)"), AveCount, ((float)PoseCount - (float)AveCount) / (float)PoseCount * 100.0f);
	const FString MaxMessage = FString::Printf(TEXT("High: %02d (%f % Reduction)"), MaxCount, ((float)PoseCount - (float)MaxCount) / (float)PoseCount * 100.0f);
	const FString MinMessage = FString::Printf(TEXT("Low: %02d (%f % Reduction)"), MinCount, ((float)PoseCount - (float)MinCount) / (float)PoseCount * 100.0f);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(TotalMessage, FColor::Black);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(LastMessage, FColor::Purple);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(AveMessage, FColor::Blue);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(MaxMessage, FColor::Red);
	AnimInstanceProxy->AnimDrawDebugOnScreenMessage(MinMessage, FColor::Green);
}