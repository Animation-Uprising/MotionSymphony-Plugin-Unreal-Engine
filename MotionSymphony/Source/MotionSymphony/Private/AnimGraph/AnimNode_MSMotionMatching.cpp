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

	void FMotionMatchingInputData::Empty(const int32 Size)
	{
		DesiredInputArray.Empty(Size);
	}

	FAnimNode_MSMotionMatching::FAnimNode_MSMotionMatching() :
		UpdateInterval(0.1f),
		PlaybackRate(1.0f),
		BlendTime(0.3f),
		OverrideQualityVsResponsivenessRatio(0.5f),
		MotionData(nullptr),
		UserCalibration(nullptr),
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

		InputData.Empty(21);
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
			MotionRecorderNode->RegisterBonesToRecord(MotionData->MotionMatchConfig->PoseBones);
		}
		
		//Create the bone remap for runtime retargeting
		USkeletalMesh* SkeletalMesh = Context.AnimInstanceProxy->GetSkelMeshComponent()->SkeletalMesh;
		if (!SkeletalMesh)
		{
			return;
		}

		const FReferenceSkeleton& AnimBPRefSkeleton = Context.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		const FReferenceSkeleton& SkelMeshRefSkeleton = SkeletalMesh->GetRefSkeleton();
		
		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

		PoseBoneRemap.Empty(MMConfig->PoseBones.Num() + 1);
		for (int32 i = 0; i < MMConfig->PoseBones.Num(); ++i)
		{
			FName BoneName = AnimBPRefSkeleton.GetBoneName(MMConfig->PoseBones[i].BoneIndex);
			int32 RemapBoneIndex = SkelMeshRefSkeleton.FindBoneIndex(BoneName);

			PoseBoneRemap.Add(RemapBoneIndex);
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

		//Todo: DATA DRIVEN - Add this back in when the motion recorder is data driven
		// if (MotionRecorderNode)
		// {
		// 	ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
		// 	ScheduleTransitionPoseSearch(Context);
		// }
		// else
		// {
			//We just jump to the default pose because there is no way to match to external nodes.
			JumpToPose(0);
		//}
	}

	void FAnimNode_MSMotionMatching::InitializeDistanceMatching(const FAnimationUpdateContext& Context)
	{
		if (bBlendTrajectory)
		{
			ApplyTrajectoryBlending();
		}

		//Filter the appropriate Distance Match Groups
		FDistanceMatchGroup* DistanceMatchGroup = MotionData->DistanceMatchSections.Find(FDistanceMatchIdentifier(DistanceMatchPayload.MatchType, DistanceMatchPayload.MatchBasis));

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
			const int32 PoseId = Section.StartPoseId + FMath::RoundToInt(MatchTime / MotionData->PoseInterval);
			FPoseMotionData& Pose = MotionData->Poses[PoseId];

			//Todo: Update with data driven 
			//Trajectory Cost
			// float Cost = FMotionMatchingUtils::ComputeTrajectoryCost(DesiredTrajectory.TrajectoryPoints,
			//                                                          Pose.Trajectory, FinalCalibration) * OverrideTrajMultiplier;
			//
			// //Pose Cost
			// Cost += FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData,
			// 	Pose.JointData, FinalCalibration) * OverridePoseMultiplier;
			//
			// //Body Velocity Cost
			// Cost += FVector::DistSquared(CurrentInterpolatedPose.LocalVelocity, Pose.LocalVelocity)
			// 	* FinalCalibration.Weight_Momentum * OverridePoseMultiplier;
			//
			// //Rotational Momentum Cost
			// Cost += FMath::Abs(CurrentInterpolatedPose.RotationalVelocity - Pose.RotationalVelocity)
			// 	* FinalCalibration.Weight_AngularMomentum * OverridePoseMultiplier;
			//
			// Cost *= Pose.Favour;
			//
			// if (Cost < LowestCost)
			// {
			// 	LowestCost = Cost;
			// 	LowestCostTime = MatchTime;
			// 	LowestCostSection = &Section;
			// 	LowestCostPoseId = PoseId;
			// 	LowestLastKeyChecked = LastKeyChecked;
			// }
		}

		//Whatever pose at the desired distance has the lowest cost, use that pose for distance matching
		if (LowestCostSection)
		{
			ActiveDistanceMatchSection = LowestCostSection;
			MotionMatchingMode = EMotionMatchingMode::DistanceMatching;
			DistanceMatchTime = LowestCostTime;
			LastDistanceMatchKeyChecked = LowestLastKeyChecked;

			const FPoseMotionData& Pose = MotionData->Poses[LowestCostPoseId];

			TransitionToPose(LowestCostPoseId, Context, LowestCostTime - Pose.Time);
		}
	}

	void FAnimNode_MSMotionMatching::InitializeMotionAction(const FAnimationUpdateContext& Context)
	{
		//Calculate how many poses prior to the action to use
		const int32 PoseOffsetToStart = FMath::Abs(FMath::RoundHalfFromZero(MotionActionPayload.LeadLength / MotionData->PoseInterval));

		if(!FinalCalibrationSets.Contains(RequiredTraits))
		{
			return;
		}

		const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];

		//Cost function on action poses
		int32 BestPoseId = -1;
		int32 BestActionId = -1;
		float BestActionCost = 10000000.0f;
		for (int32 i = 0; i < MotionData->Actions.Num(); ++i)
		{
			const FMotionAction& MotionAction = MotionData->Actions[i];

			if (MotionAction.ActionId == MotionActionPayload.ActionId) //TODO: Support Traits?
			{
				const int32 PoseId = FMath::Clamp(MotionAction.PoseId - PoseOffsetToStart, 0, MotionData->Poses.Num());
				FPoseMotionData& Pose = MotionData->Poses[PoseId];

				//Todo: Update with data driven
				// float Cost = FMotionMatchingUtils::ComputePoseCost(Pose.JointData, CurrentInterpolatedPose.JointData, FinalCalibration);
				//
				// //TODO: Add momentum
				// //TODO: Add rotational momentum
				// //TODO: Add trajectory
				//
				// Cost *= Pose.Favour;
				//
				// if (Cost < BestActionCost)
				// {
				// 	BestActionCost = Cost;
				// 	BestActionId = i;
				// 	BestPoseId = Pose.PoseId;
				// }
			}
		}

		if (BestActionId > -1)
		{
			TransitionToPose(BestPoseId, Context);
			MotionMatchingMode = EMotionMatchingMode::Action;
			CurrentActionId = BestActionId;
			CurrentActionTime = MotionData->Actions[BestActionId].Time;
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

		//Todo: DATA DRIVEN - Add this back in when the MotionRecorder is data oriented
		// if (MotionRecorderNode)
		// {
		// 	ComputeCurrentPose(MotionRecorderNode->GetMotionPose());
		// }
		// else
		// {
			ComputeCurrentPose();
		//}

		//If we have ran into a 'DoNotUse' pose. We need to force a new pose search
		if(CurrentInterpolatedPose.bDoNotUse)
		{
			bForcePoseSearch = true;
		}

		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

		//Past trajectory mode
		if (PastTrajectoryMode == EPastTrajectoryMode::CopyFromCurrentPose)
		{
			const int32 ResponseArraySize = FMath::Min3(InputData.DesiredInputArray.Num(),
				MotionData->MotionMatchConfig->ResponseDimensionCount, CurrentInterpolatedPoseArray.Num());
			for (int32 i = 0; i < ResponseArraySize; ++i)
			{
				if (MMConfig->TrajectoryTimes[i] > 0.0f)
				{ 
					break;
				}

				InputData.DesiredInputArray[i] = CurrentInterpolatedPoseArray[i];
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
		const float PoseInterval = FMath::Max(0.01f, MotionData->PoseInterval);

		//====== Determine the next chosen pose ========
		const FAnimChannelState& ChosenChannel = BlendChannels.Last();

		float ChosenClipLength = 0.0f;
		switch (ChosenChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence: ChosenClipLength = MotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::BlendSpace: ChosenClipLength = MotionData->GetSourceBlendSpaceAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::Composite: ChosenClipLength = MotionData->GetSourceCompositeAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
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
			case EMotionAnimAssetType::Sequence: DominantClipLength = MotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::BlendSpace: DominantClipLength = MotionData->GetSourceBlendSpaceAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::Composite: DominantClipLength = MotionData->GetSourceCompositeAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
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

		FPoseMatrix& PoseMatrix = MotionData->PoseMatrix;
		TArray<float>& PoseArray = PoseMatrix.PoseArray;

		FMotionMatchingUtils::LerpFloatArray(CurrentInterpolatedPoseArray, &PoseArray[PoseMatrix.AtomCount * BeforePose->PoseId],
			&PoseArray[PoseMatrix.AtomCount * AfterPose->PoseId], PoseInterpolationValue);

		//Inject the input array / trajectory
		for(int32 i = 0; i < InputData.DesiredInputArray.Num(); ++i)
		{
			CurrentInterpolatedPoseArray[i] = InputData.DesiredInputArray[i];
		}
	}

	void FAnimNode_MSMotionMatching::ComputeCurrentPose(const FCachedMotionPose& CachedMotionPose)
	{
		const float PoseInterval = FMath::Max(0.01f, MotionData->PoseInterval);

		//====== Determine the next chosen pose ========
		const FAnimChannelState& ChosenChannel = BlendChannels.Last();

		float ChosenClipLength = 0.0f;
		switch (ChosenChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence: ChosenClipLength = MotionData->GetSourceAnimAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::BlendSpace: ChosenClipLength = MotionData->GetSourceBlendSpaceAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::Composite: ChosenClipLength = MotionData->GetSourceCompositeAtIndex(ChosenChannel.AnimId).GetPlayLength(); break;
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
			case EMotionAnimAssetType::Sequence: DominantClipLength = MotionData->GetSourceAnimAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::BlendSpace: DominantClipLength = MotionData->GetSourceBlendSpaceAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
			case EMotionAnimAssetType::Composite: DominantClipLength = MotionData->GetSourceCompositeAtIndex(DominantChannel.AnimId).GetPlayLength(); break;
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

		const int32 MaxPoseIndex = MotionData->Poses.Num() - 1;
		PoseIndex = FMath::Clamp(PoseIndex + NumPosesPassed, 0, MaxPoseIndex);

		//Get the before and after poses and then interpolate
		FPoseMotionData* BeforePose;
		FPoseMotionData* AfterPose;

		if (TimePassed < -0.00001f)
		{
			AfterPose = &MotionData->Poses[PoseIndex];
			BeforePose = &MotionData->Poses[FMath::Clamp(AfterPose->LastPoseId, 0, MaxPoseIndex)];

			PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - (float)NumPosesPassed);
		}
		else
		{
			BeforePose = &MotionData->Poses[FMath::Min(PoseIndex, MotionData->Poses.Num() - 2)];
			AfterPose = &MotionData->Poses[FMath::Clamp(BeforePose->NextPoseId, 0, MaxPoseIndex)];

			PoseInterpolationValue = (TimePassed / PoseInterval) - (float)NumPosesPassed;
		}

		PoseInterpolationValue = FMath::Clamp(PoseInterpolationValue, 0.0f, 1.0f);
		
		FMotionMatchingUtils::LerpPose(CurrentInterpolatedPose, *BeforePose,
			*AfterPose, PoseInterpolationValue);

		FPoseMatrix& PoseMatrix = MotionData->PoseMatrix;
		TArray<float>& PoseArray = PoseMatrix.PoseArray;

		FMotionMatchingUtils::LerpFloatArray(CurrentInterpolatedPoseArray, &PoseArray[PoseMatrix.AtomCount * BeforePose->PoseId],
			&PoseArray[PoseMatrix.AtomCount * AfterPose->PoseId], PoseInterpolationValue);

		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

		//Todo: Fix for data driven - This is needed for the motion snapshot to work! Inject the snapshotted pose data into the array
		// for (int32 i = 0; i < PoseBoneRemap.Num(); ++i)
		// {
		// 	const FCachedMotionBone& CachedMotionBone = CachedMotionPose.CachedBoneData[PoseBoneRemap[i]];
		// 	//CurrentInterpolatedPose.JointData[i] = FJointData(CachedMotionBone.Transform.GetLocation(), CachedMotionBone.Velocity);
		// }

		//Inject the input array / trajectory
		for(int32 i = 0; i < InputData.DesiredInputArray.Num(); ++i)
		{
			CurrentInterpolatedPoseArray[i] = InputData.DesiredInputArray[i];
		}
	}

	void FAnimNode_MSMotionMatching::SchedulePoseSearch(const FAnimationUpdateContext& Context)
	{
		if (bBlendTrajectory)
		{
			ApplyTrajectoryBlending();
		}

		const int32 MaxPoseId = MotionData->Poses.Num() - 1;
		CurrentChosenPoseId = FMath::Clamp(CurrentChosenPoseId, 0, MaxPoseId); //Just in case
		int32 NextPoseId = MotionData->Poses[CurrentChosenPoseId].NextPoseId;
		if(NextPoseId < 0)
		{
			NextPoseId = CurrentChosenPoseId;
		}
		
		FPoseMotionData& NextPose = MotionData->Poses[FMath::Clamp(NextPoseId, 0, MaxPoseId)];

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

		const FPoseMotionData& BestPose = MotionData->Poses[LowestPoseId];
		const FPoseMotionData& ChosenPose = MotionData->Poses[CurrentChosenPoseId];

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

		LowestPoseId = FMath::Clamp(LowestPoseId, 0, MotionData->Poses.Num() - 1);
		JumpToPose(LowestPoseId);
	}

	int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId()
	{
		if (!FinalCalibrationSets.Contains(RequiredTraits))
		{
			return CurrentChosenPoseId;
		}
		
		CalibrationArray = FinalCalibrationSets[RequiredTraits].Weights;

		int32 LowestPoseId = 0;
		float LowestCost = 10000000.0f;
		
		const int32 PoseCount = MotionData->PoseMatrix.PoseCount;
		const int32 AtomCount = MotionData->PoseMatrix.AtomCount;
		TArray<float>& PoseArray = MotionData->PoseMatrix.PoseArray; 
		
		//This loop is tight, no pointers allowed?
		for(int32 PoseIndex = 0; PoseIndex < PoseCount; ++PoseIndex)
		{
			float Cost = 0.0f;
			const float PoseFavour = 1.0f; //Todo: Take this from the first index of the pose and start inner loop at index 1

			for(int32 AtomIndex = 0; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[PoseIndex * AtomCount + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex] * PoseFavour;
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId = PoseIndex;
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

		//Todo: This final calibration set should not be needed
		const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];
		TArray<FPoseMotionData>* PoseCandidates = MotionData->OptimisationModule->GetFilteredPoseList(CurrentInterpolatedPose, RequiredTraits, FinalCalibration);

		if (!PoseCandidates)
		{
			return GetLowestCostPoseId_Linear(NextPose);
		}

		//Todo: Bake these into the calibration array
		GenerateCalibrationArray();
		
		const int32 PoseCount = MotionData->PoseMatrix.PoseCount;
		const int32 AtomCount = MotionData->PoseMatrix.AtomCount;
		TArray<float>& PoseArray = MotionData->PoseMatrix.PoseArray; //Todo: PRIORITY! This needs to be taken from the optimisation module not directly from motion data
		
		//This loop is tight, no pointers allowed
		int32 LowestPoseId = 0;
		float LowestCost = 10000000.0f;
		for(int32 PoseIndex = 0; PoseCount; ++PoseIndex)
		{
			float Cost = 0.0f;
			const float PoseFavour = 1.0f; //Todo: Take this from the first index of the pose and start inner loop at index 1

			for(int32 AtomIndex = 0; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[PoseIndex * AtomCount + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex] * PoseFavour;
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId = PoseIndex;
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
		
		GenerateCalibrationArray();

		const int32 PoseCount = MotionData->PoseMatrix.PoseCount;
		const int32 AtomCount = MotionData->PoseMatrix.AtomCount;
		TArray<float>& PoseArray = MotionData->PoseMatrix.PoseArray; 

		//This loop is tight, no pointers allowed?
		int32 LowestPoseId = 0;
		float LowestCost = 10000000.0f;
		for(int32 PoseIndex = 0; PoseIndex < PoseCount; ++PoseIndex)
		{
			float Cost = 0.0f;
			const float PoseFavour = 1.0f; //Todo: Take this from the first index of the pose and start inner loop at index 1

			for(int32 AtomIndex = 0; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[PoseIndex * AtomCount + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex] * PoseFavour;
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId = PoseIndex;
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

		BlendChannels.Empty(TransitionMethod == ETransitionMethod::Blend ? 12 : 1);
		const FPoseMotionData& Pose = MotionData->Poses[PoseId];

		switch (Pose.AnimType)
		{
			//Sequence Pose
			case EMotionAnimAssetType::Sequence: 
			{
				const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(Pose.AnimId);

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
				const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

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
				const FMotionComposite& MotionComposite = MotionData->GetSourceCompositeAtIndex(Pose.AnimId);

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

		const FPoseMotionData& Pose = MotionData->Poses[PoseId];

		switch (Pose.AnimType)
		{
			//Sequence Pose
			case EMotionAnimAssetType::Sequence:
			{
				const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(Pose.AnimId);

				BlendChannels.Emplace(FAnimChannelState(Pose, EBlendStatus::Chosen, 1.0f,
					MotionAnim.Sequence->GetPlayLength(), MotionAnim.bLoop, MotionAnim.PlayRate,
					Pose.bMirrored, TimeSinceMotionChosen, TimeOffset));

			} break;
			//Blend Space Pose
			case EMotionAnimAssetType::BlendSpace:
			{
				const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(Pose.AnimId);

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
				const FMotionComposite& MotionComposite = MotionData->GetSourceCompositeAtIndex(Pose.AnimId);

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

	bool FAnimNode_MSMotionMatching::NextPoseToleranceTest(const FPoseMotionData& NextPose)
	{
		if (NextPose.bDoNotUse 
		|| NextPose.Traits != RequiredTraits)
		{
			return false;
		}
		
		const int32 NextPoseStartIndex = NextPose.PoseId * MotionData->PoseMatrix.AtomCount;

		int32 FeatureOffset = 0;
		for(const TObjectPtr<UMatchFeatureBase> Feature : MotionData->MotionMatchConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				if(!Feature->NextPoseToleranceTest(InputData.DesiredInputArray,MotionData->PoseMatrix.PoseArray,
					NextPoseStartIndex + FeatureOffset, FeatureOffset, PositionTolerance, RotationTolerance))
				{
					return false;
				}
			}

			FeatureOffset += Feature->Size();
		}

		return true;
	}

	void FAnimNode_MSMotionMatching::ApplyTrajectoryBlending()
	{
		int32 FeatureOffset = 0;
		for(const TObjectPtr<UMatchFeatureBase> Feature : MotionData->MotionMatchConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				Feature->ApplyInputBlending(InputData.DesiredInputArray, CurrentInterpolatedPoseArray,
					FeatureOffset, TrajectoryBlendMagnitude);
			}

			FeatureOffset += Feature->Size();
		}
	}

	void FAnimNode_MSMotionMatching::GenerateCalibrationArray()
	{
		const float OverrideQualityMultiplier = (1.0f - OverrideQualityVsResponsivenessRatio) * 2.0f;
		const float OverrideResponseMultiplier = OverrideQualityVsResponsivenessRatio * 2.0f;
		CalibrationArray = FinalCalibrationSets[RequiredTraits].Weights;

		int32 AtomIndex = 0;
		for(TObjectPtr<UMatchFeatureBase> FeaturePtr : MotionData->MotionMatchConfig->Features)
		{
			const UMatchFeatureBase* Feature = FeaturePtr.Get();
			const int32 FeatureSize = Feature->Size();

			if(Feature->PoseCategory == EPoseCategory::Quality)
			{
				for(int32 i = 0; i < FeatureSize; ++i)
				{
					CalibrationArray[AtomIndex] *= OverrideQualityMultiplier;
					++AtomIndex;
				}
			}
			else
			{
				for(int32 i = 0; i < FeatureSize; ++i)
				{
					CalibrationArray[AtomIndex] *= OverrideResponseMultiplier;
					++AtomIndex;
				}
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
		return MotionData;
	}

	bool FAnimNode_MSMotionMatching::NeedsOnInitializeAnimInstance() const
	{
		return true;
	}

	void FAnimNode_MSMotionMatching::OnInitializeAnimInstance(const FAnimInstanceProxy* InAnimInstanceProxy, const UAnimInstance* InAnimInstance)
	{
		Super::OnInitializeAnimInstance(InAnimInstanceProxy, InAnimInstance);

		bValidToEvaluate = true;

		//Validate Motion Data
		if (!MotionData)
		{
			UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Motion Data has not been set."))
			bValidToEvaluate = false;
			return;
		}

		//Validate Motion Matching Configuration
		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;
		if (MMConfig)
		{
			MMConfig->Initialize();

			const int32 PoseArraySize = MotionData->PoseMatrix.AtomCount;

			CurrentInterpolatedPose = FPoseMotionData();
			CurrentInterpolatedPoseArray.Empty(PoseArraySize + 1);
			CalibrationArray.Empty(PoseArraySize + 1);

			CurrentInterpolatedPoseArray.SetNumZeroed(PoseArraySize);
			InputData.DesiredInputArray.SetNumZeroed(PoseArraySize);
			CalibrationArray.SetNumZeroed(PoseArraySize);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Motion matching node failed to initialize. Motion Match Config has not been set on the motion matching node."));
			bValidToEvaluate = false;
			return;
		}

		//Validate MMConfig matches internal calibration (i.e. the MMConfig has not been since changed)
		for(auto& TraitCalibPair : MotionData->FeatureStandardDeviations)
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
			&& MotionData->IsOptimisationValid())
		{
			MotionData->OptimisationModule->InitializeRuntime();
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Motion matching node was set to run in optimized mode. However, the optimisation setup is invalid and optimization will be disabled. Did you forget to pre-process your motion data with optimisation on?"));
			PoseMatchMethod = EPoseMatchMethod::Linear;
		}

		//If the user calibration is not set on the motion data asset, get it from the Motion Data instead
		if (!UserCalibration)
		{
			UserCalibration = MotionData->PreprocessCalibration;
		}

		if (UserCalibration)
		{
			UserCalibration->ValidateData();

			for (auto& FeatureStdDevPairs : MotionData->FeatureStandardDeviations)
			{
				FCalibrationData& NewFinalCalibration = FinalCalibrationSets.Add(FeatureStdDevPairs.Key, FCalibrationData());
				NewFinalCalibration.GenerateFinalWeights(UserCalibration, FeatureStdDevPairs.Value);
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

		MirroringData.Initialize(MotionData->MirroringProfile, InAnimInstanceProxy->GetSkelMeshComponent());
	}

	void FAnimNode_MSMotionMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
	{
		DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
		FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
		
		GetEvaluateGraphExposedInputs().Execute(Context);

		InternalTimeAccumulator = 0.0f;

		if (!MotionData 
		|| !MotionData->bIsProcessed
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

		if (!MotionData 
		|| !MotionData->bIsProcessed
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

			//Draw Input ArrayFeatures
			DrawInputArrayDebug(Context.AnimInstanceProxy);
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
		
		if (!MotionData 
		|| !MotionData->bIsProcessed
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

		switch (PrimaryChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence:
			{
				const FMotionAnimSequence& MotionSequence = MotionData->GetSourceAnimAtIndex(PrimaryChannel.AnimId);
				UAnimSequence* AnimSequence = MotionSequence.Sequence;


				if(MotionSequence.bLoop)
				{
					AnimTime = FMotionMatchingUtils::WrapAnimationTime(AnimTime, AnimSequence->GetPlayLength());
				}
					
				FAnimationPoseData AnimationPoseData(Output);
				AnimSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(AnimTime, true));
			} break;

			case EMotionAnimAssetType::BlendSpace:
			{
				const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(PrimaryChannel.AnimId);
				UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;

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
				const FMotionComposite& MotionComposite = MotionData->GetSourceCompositeAtIndex(PrimaryChannel.AnimId);
				UAnimComposite* Composite = MotionComposite.AnimComposite;

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
			FMotionMatchingUtils::MirrorPose(Output.Pose, MotionData->MirroringProfile, MirroringData, 
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
						const FMotionAnimSequence& MotionAnim = MotionData->GetSourceAnimAtIndex(AnimChannel.AnimId);
						UAnimSequence* AnimSequence = MotionAnim.Sequence;

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
						const FMotionBlendSpace& MotionBlendSpace = MotionData->GetSourceBlendSpaceAtIndex(AnimChannel.AnimId);
						UBlendSpace* BlendSpace = MotionBlendSpace.BlendSpace;

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
					FMotionMatchingUtils::MirrorPose(Pose, MotionData->MirroringProfile, MirroringData, Output.AnimInstanceProxy->GetSkelMeshComponent());
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
				const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim();

				if(PrimaryAnim)
				{
					FAnimationPoseData AnimationPoseData(Output);
					PrimaryAnim->GetAnimationPose(AnimationPoseData, FAnimExtractContext(BlendChannels.Last().AnimTime, true));
				}
			}
		}
		else
		{
			const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim();
			if (PrimaryAnim)
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
		
		SyncScope.AddTickRecord(TickRecord, SyncParams, UE::Anim::FAnimSyncDebugInfo(Context));
	    
		TRACE_ANIM_TICK_RECORD(Context, TickRecord);
	}

	void FAnimNode_MSMotionMatching::PerformLinearSearchComparison(const FAnimationUpdateContext& Context, int32 ComparePoseId, FPoseMotionData& NextPose)
	{
		int32 LowestPoseId = LowestPoseId = GetLowestCostPoseId_Linear(NextPose);

		const bool SamePoseChosen = LowestPoseId == ComparePoseId;

		LowestPoseId = FMath::Clamp(LowestPoseId, 0, MotionData->Poses.Num() - 1);

		float LinearChosenPoseCost = 0.0f;
		const float ActualChosenPoseCost = 0.0f;

		float LinearChosenTrajectoryCost = 0.0f;
		float ActualChosenTrajectoryCost = 0.0f;

		//Todo: Update for data driven
		// if (!SamePoseChosen)
		// {
		// 	const FPoseMotionData& LinearPose = MotionData->Poses[LowestPoseId];
		// 	const FPoseMotionData& ActualPose = MotionData->Poses[ComparePoseId];
		//
		// 	LinearChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, LinearPose.Trajectory,
		// 		1.0f, 0.0f);
		//
		// 	ActualChosenTrajectoryCost = FMotionMatchingUtils::ComputeTrajectoryCost(CurrentInterpolatedPose.Trajectory, ActualPose.Trajectory,
		// 		1.0f, 0.0f);
		//
		// 	LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData, LinearPose.JointData,
		// 		1.0f, 0.0f);
		//
		// 	LinearChosenPoseCost = FMotionMatchingUtils::ComputePoseCost(CurrentInterpolatedPose.JointData, ActualPose.JointData,
		// 		1.0f, 0.0f);
		// }

		const UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

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

		return MotionData->GetSourceAnimAtIndex(AnimChannel.AnimId).Sequence;
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
			case EMotionAnimAssetType::Sequence: return MotionData->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
			case EMotionAnimAssetType::Composite: return MotionData->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
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
			case EMotionAnimAssetType::Sequence: return MotionData->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
			case EMotionAnimAssetType::Composite: return MotionData->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
			default: return nullptr;
		}
	}

	void FAnimNode_MSMotionMatching::DrawInputArrayDebug(FAnimInstanceProxy* InAnimInstanceProxy)
	{
		if(!InAnimInstanceProxy
			|| InputData.DesiredInputArray.Num() == 0)
		{
			return;
		}

		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;
		
		const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
		const FVector ActorLocation = MeshTransform.GetLocation();
		
		int32 FeatureOffset = 0;
		for(const TObjectPtr<UMatchFeatureBase> Feature : MMConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				Feature->DrawDebugDesiredRuntime(InAnimInstanceProxy, InputData, FeatureOffset, MMConfig);
			}

			FeatureOffset += Feature->Size();
		}
	}

	void FAnimNode_MSMotionMatching::DrawChosenTrajectoryDebug(FAnimInstanceProxy* InAnimInstanceProxy)
	{
		if (InAnimInstanceProxy == nullptr 
		|| CurrentChosenPoseId > MotionData->Poses.Num() - 1)
		{
			return;
		}

		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

		int32 FeatureOffset = 0;
		for(const TObjectPtr<UMatchFeatureBase> Feature : MMConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				Feature->DrawDebugCurrentRuntime(InAnimInstanceProxy,MotionData, CurrentInterpolatedPoseArray, FeatureOffset);
			}

			FeatureOffset += Feature->Size();
		}
	}

	void FAnimNode_MSMotionMatching::DrawChosenPoseDebug(FAnimInstanceProxy* InAnimInstanceProxy, bool bDrawVelocity)
	{
		if (InAnimInstanceProxy == nullptr 
		|| CurrentChosenPoseId > MotionData->Poses.Num() - 1)
		{
			return;
		}

		UMotionMatchConfig* MMConfig = MotionData->MotionMatchConfig;

		int32 FeatureOffset = 0;
		for(const TObjectPtr<UMatchFeatureBase> Feature : MMConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Quality)
			{
				Feature->DrawDebugCurrentRuntime(InAnimInstanceProxy,MotionData, CurrentInterpolatedPoseArray, FeatureOffset);
			}

			FeatureOffset += Feature->Size();
		}
		
		//Todo: Update for data driven
		// FPoseMotionData& ChosenPose = CurrentInterpolatedPose;
		// TArray<FJointData>& PoseJoints = ChosenPose.JointData;
		//
		// if (PoseJoints.Num() == 0)
		// {
		// 	return;
		// }
		//
		// const FTransform& ActorTransform = InAnimInstanceProxy->GetActorTransform();
		// const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
		// FVector ActorLocation = MeshTransform.GetLocation();
		//
		// //Draw Body Velocity
		// InAnimInstanceProxy->AnimDrawDebugSphere(ActorLocation, 5.0f, 32, FColor::Blue, false, -1.0f, 0.0f);
		// InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(ActorLocation, MeshTransform.TransformPosition(
		// 	ChosenPose.LocalVelocity), 80.0f, FColor::Blue, false, -1.0f, 3.0f);

		// for (int32 i = 0; i < PoseJoints.Num(); ++i)
		// {
		// 	FJointData& Joint = PoseJoints[i];
		//
		// 	FColor Color = FColor::Yellow;
		//
		// 	FVector JointPosition = MeshTransform.TransformPosition(Joint.Position);
		// 	InAnimInstanceProxy->AnimDrawDebugSphere(JointPosition, 5.0f, 32, Color, false, -1.0f, 0.0f);
		//
		// 	if (bDrawVelocity)
		// 	{
		// 		FVector DrawTo = MeshTransform.TransformPosition(Joint.Position + (Joint.Velocity * 0.33333f));
		// 		InAnimInstanceProxy->AnimDrawDebugDirectionalArrow(JointPosition, DrawTo, 40.0f, Color, false, -1.0f, 2.0f);
		// 	}
		// }
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

		//Todo: Update for Data driven
		
		// FVector LastPoint = CharTransform.TransformPosition(Pose.Trajectory[0].Position);
		// LastPoint.Z -= 87.0f;
		//
		// for (int32 i = 1; i < Pose.Trajectory.Num(); ++i)
		// {
		// 	FVector ThisPoint = CharTransform.TransformPosition(Pose.Trajectory[i].Position);
		// 	ThisPoint.Z -= 87.0f;
		// 	InAnimInstanceProxy->AnimDrawDebugLine(LastPoint, ThisPoint, FColor::Orange, false, 0.1f, 1.0f);
		// 	LastPoint = ThisPoint;
		// }
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

		const int32 PoseCount = MotionData->Poses.Num();

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

		FPoseMotionData& CurrentPose = MotionData->Poses[FMath::Clamp(CurrentInterpolatedPose.PoseId,
				0, MotionData->Poses.Num())];

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
		
		FMotionAnimAsset* MotionAnimAsset = MotionData->GetSourceAnim(CurrentPose.AnimId, CurrentPose.AnimType);
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