	// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "AnimGraph/AnimNode_MSMotionMatching.h"
#include "AITypes.h"
#include "AnimationRuntime.h"
#include "Animation/AnimSequence.h"
#include "DrawDebugHelpers.h"
#include "ModuleDescriptor.h"
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

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
		
		if(MotionRecorderNode)
		{
			MotionRecorderNode->RegisterMotionMatchConfig(MMConfig);
		}

		//Todo: Convert this to Data Oriented later
		//Create the bone remap for runtime retargeting
		// USkeletalMesh* SkeletalMesh = Context.AnimInstanceProxy->GetSkelMeshComponent()->GetSkeletalMeshAsset();
		// if (!SkeletalMesh)
		// {
		// 	return;
		// }
		
		//const FReferenceSkeleton& AnimBPRefSkeleton = Context.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		//const FReferenceSkeleton& SkelMeshRefSkeleton = SkeletalMesh->GetRefSkeleton();
		
		// PoseBoneRemap.Empty(MMConfig->PoseBones.Num() + 1);
		// for (int32 i = 0; i < MMConfig->PoseBones.Num(); ++i)
		// {
		// 	FName BoneName = AnimBPRefSkeleton.GetBoneName(MMConfig->PoseBones[i].BoneIndex);
		// 	int32 RemapBoneIndex = SkelMeshRefSkeleton.FindBoneIndex(BoneName);
		//
		// 	PoseBoneRemap.Add(RemapBoneIndex);
		// }
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
		if (MotionRecorderNode)
		{
			ComputeCurrentPose(MotionRecorderNode->GetMotionPose(), MotionRecorderNode->GetCurrentPoseArray());
		 	ScheduleTransitionPoseSearch(Context);
		}else
		{
			//We just jump to the default pose because there is no way to match to external nodes.
			JumpToPose(0);
		}
	}


	void FAnimNode_MSMotionMatching::UpdateMotionMatchingState(const float DeltaTime, const FAnimationUpdateContext& Context)
	{
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
		if (IMotionSnapper* MotionSnapper = Context.GetMessage<IMotionSnapper>())
		{
			MotionRecorderNode = &MotionSnapper->GetNode();
		}
		
		if (MotionRecorderNode)
		{
		 	ComputeCurrentPose(MotionRecorderNode->GetMotionPose(), MotionRecorderNode->GetCurrentPoseArray());
		}
		else
		{
			ComputeCurrentPose();
		}
		
		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		bForcePoseSearch = CheckForcePoseSearch(CurrentMotionData);
		
		//Past trajectory mode
		UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
		if (PastTrajectoryMode == EPastTrajectoryMode::CopyFromCurrentPose)
		{
			int32 CurrentOffset = 0;
		 	
		 	for(const TObjectPtr<UMatchFeatureBase> FeaturePtr : MMConfig->Features)
		 	{
		 		if(FeaturePtr->PoseCategory == EPoseCategory::Responsiveness)
		 		{
		 			const int32 FeatureLimit = FMath::Min(FeaturePtr->Size() + CurrentOffset, InputData.DesiredInputArray.Num());
		 			for(int32 i = CurrentOffset; i < FeatureLimit; ++i)
		 			{
		 				InputData.DesiredInputArray[i] = CurrentInterpolatedPoseArray[i];
		 			}
		 		}
		 		CurrentOffset += FeaturePtr->Size();
		 	}
		}
		
		if (TimeSinceMotionUpdate >= UpdateInterval || bForcePoseSearch)
		{
			TimeSinceMotionUpdate = 0.0f;
			PoseSearch(Context);
		}
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

		int32 NumPosesPassed;
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

			PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - static_cast<float>(NumPosesPassed));
		}
		else
		{
			BeforePose = &CurrentMotionData->Poses[FMath::Min(PoseIndex, CurrentMotionData->Poses.Num() - 2)];
			AfterPose = &CurrentMotionData->Poses[BeforePose->NextPoseId];

			PoseInterpolationValue = (TimePassed / PoseInterval) - static_cast<float>(NumPosesPassed);
		}

		FMotionMatchingUtils::LerpPose(CurrentInterpolatedPose, *BeforePose, *AfterPose, PoseInterpolationValue);

		FPoseMatrix& PoseMatrix = CurrentMotionData->LookupPoseMatrix;
		TArray<float>& PoseArray = PoseMatrix.PoseArray;

		FMotionMatchingUtils::LerpFloatArray(CurrentInterpolatedPoseArray, &PoseArray[PoseMatrix.AtomCount * BeforePose->PoseId],
			&PoseArray[PoseMatrix.AtomCount * AfterPose->PoseId], PoseInterpolationValue);

		//Inject the input array / trajectory
		CurrentInterpolatedPoseArray[0] = 1.0f;
		for(int32 i = 0; i < InputData.DesiredInputArray.Num(); ++i)
		{
			CurrentInterpolatedPoseArray[i+1] = InputData.DesiredInputArray[i];
		}
	}

	void FAnimNode_MSMotionMatching::ComputeCurrentPose(const FCachedMotionPose& CachedMotionPose, const TArray<float> CurrentPoseArray)
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

		int32 NumPosesPassed;
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

			PoseInterpolationValue = 1.0f - FMath::Abs((TimePassed / PoseInterval) - static_cast<float>(NumPosesPassed));
		}
		else
		{
			BeforePose = &CurrentMotionData->Poses[FMath::Min(PoseIndex, CurrentMotionData->Poses.Num() - 2)];
			AfterPose = &CurrentMotionData->Poses[FMath::Clamp(BeforePose->NextPoseId, 0, MaxPoseIndex)];

			PoseInterpolationValue = (TimePassed / PoseInterval) - static_cast<float>(NumPosesPassed);
		}

		PoseInterpolationValue = FMath::Clamp(PoseInterpolationValue, 0.0f, 1.0f);
		
		FMotionMatchingUtils::LerpPose(CurrentInterpolatedPose, *BeforePose,
			*AfterPose, PoseInterpolationValue);

		FPoseMatrix& PoseMatrix = CurrentMotionData->LookupPoseMatrix;
		TArray<float>& PoseArray = PoseMatrix.PoseArray;

		FMotionMatchingUtils::LerpFloatArray(CurrentInterpolatedPoseArray, &PoseArray[PoseMatrix.AtomCount * BeforePose->PoseId], 
			&PoseArray[PoseMatrix.AtomCount * AfterPose->PoseId], PoseInterpolationValue);

		//Inject the input array / trajectory. This currently assumes that all input is first in the pose array
		CurrentInterpolatedPoseArray[0] = 1.0f;
		for(int32 i = 0; i < InputData.DesiredInputArray.Num(); ++i)
		{
			CurrentInterpolatedPoseArray[i+1] = InputData.DesiredInputArray[i]; //+1 to pose array to make room for Pose Cost Multiplier value
		}

		int32 FeatureOffset = 1; //Start at Index 1 because index 1 is for pose cost multiplier. We don't take this from the snapshot node.
		for(const TObjectPtr<UMatchFeatureBase> Feature : CurrentMotionData->MotionMatchConfig->Features)
		{
			const int32 FeatureSize = Feature->Size();
			
			if(Feature->IsMotionSnapshotCompatible())
			{
				for(int32 i = 0; i < FeatureSize; ++i)
				{
					CurrentInterpolatedPoseArray[FeatureOffset + i] = CurrentPoseArray[FeatureOffset + i];
				}
			}

			FeatureOffset += FeatureSize;
		}
	}

	void FAnimNode_MSMotionMatching::PoseSearch(const FAnimationUpdateContext& Context)
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
		const int32 LowestPoseIdMatrix = GetLowestCostPoseId();
		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		const int32 LowestPoseIdDatabase = FMath::Clamp(CurrentMotionData->MatrixPoseIdToDatabasePoseId(
			LowestPoseIdMatrix), 0, CurrentMotionData->Poses.Num() -1);
		
		JumpToPose(LowestPoseIdDatabase);
	}

	bool FAnimNode_MSMotionMatching::CheckForcePoseSearch(const UMotionDataAsset* InMotionData) const
	{
		if(!InMotionData)
		{
			return CurrentInterpolatedPose.SearchFlag > EPoseSearchFlag::NextNatural;
		}

		const int32 PoseCountToCheck = CurrentInterpolatedPose.PoseId + FMath::CeilToInt32(BlendTime / InMotionData->GetPoseInterval());

		//End of pose data, pose search must be forced
		if(PoseCountToCheck >= InMotionData->Poses.Num())
		{
			return true;
		}

		//Check ahead to see if there will be a DoNotUse pose within the blend time or a new animation
		for(int32 i = CurrentInterpolatedPose.PoseId; i < PoseCountToCheck; ++i)
		{
			const FPoseMotionData& ThisPose = InMotionData->Poses[i];
			
			if(ThisPose.SearchFlag == EPoseSearchFlag::DoNotUse
				|| ThisPose.AnimId != CurrentInterpolatedPose.AnimId
				|| ThisPose.AnimType != CurrentInterpolatedPose.AnimType)
			{
				return true;
			}
		}

		return false;
	}

	int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId()
	{
		if (!FinalCalibrationSets.Contains(RequiredTraits))
		{
			return CurrentChosenPoseId;
		}
		
		CalibrationArray = FinalCalibrationSets[RequiredTraits].Weights;
		UMotionDataAsset* CurrentMotionData = GetMotionData();
		
		const int32 PoseCount = CurrentMotionData->SearchPoseMatrix.PoseCount;
		const int32 AtomCount = CurrentMotionData->SearchPoseMatrix.AtomCount;
		TArray<float>& PoseArray = CurrentMotionData->SearchPoseMatrix.PoseArray; 
		
		//Check cost of current pose first for "Favour Current Pose"
		int32 LowestPoseId = 0;
		float LowestCost = 10000000.0f;
		if(bFavourCurrentPose && !bForcePoseSearch)
		{
			LowestPoseId = MotionData->DatabasePoseIdToMatrixPoseId(CurrentInterpolatedPose.PoseId);
			const int32 PoseStartIndex = LowestPoseId * AtomCount;
			const float PoseFavour = PoseArray[PoseStartIndex]; //Pose cost multiplier is the first atom of a pose array

			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				LowestCost += FMath::Abs(PoseArray[PoseStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
			}

			LowestCost *= CurrentPoseFavour;
		}

		//Todo: Check cost of next natural poses which aren't included in the search loop
		

		//Main Loop Search
		for(int32 PoseIndex = 0; PoseIndex < PoseCount; ++PoseIndex)
		{
			float Cost = 0.0f;
			const int32 PoseStartIndex = PoseIndex * AtomCount;
			const float PoseFavour = PoseArray[PoseStartIndex]; //Pose cost multiplier is the first atom of a pose array
			
			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[PoseStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId = PoseIndex;
			}
		}

		return MotionData->MatrixPoseIdToDatabasePoseId(LowestPoseId);
	}

	int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId(const FPoseMotionData& NextPose)
	{
		if (!FinalCalibrationSets.Contains(RequiredTraits))
		{
			return CurrentChosenPoseId;
		}

		UMotionDataAsset* CurrentMotionData = GetMotionData();

		//Todo: This final calibration set should not be needed
		const FCalibrationData& FinalCalibration = FinalCalibrationSets[RequiredTraits];
		TArray<FPoseMotionData>* PoseCandidates = CurrentMotionData->OptimisationModule->GetFilteredPoseList(CurrentInterpolatedPose, RequiredTraits, FinalCalibration);

		if (!PoseCandidates)
		{
			return GetLowestCostPoseId_Linear(NextPose);
		}
		
		GenerateCalibrationArray();
		
		const int32 AtomCount = CurrentMotionData->SearchPoseMatrix.AtomCount;
		TArray<float>& PoseArray = CurrentMotionData->SearchPoseMatrix.PoseArray; 
		
		//Check cost of current pose first for "Favour Current Pose"
		int32 LowestPoseId = 0;
		float LowestCost = 10000000.0f;
		if(bFavourCurrentPose && !bForcePoseSearch)
		{
			LowestPoseId = CurrentMotionData->DatabasePoseIdToMatrixPoseId(CurrentInterpolatedPose.PoseId);
			const int32 PoseStartIndex = LowestPoseId * AtomCount;
			const float PoseFavour = PoseArray[PoseStartIndex]; //Pose cost multiplier is the first atom of a pose array

			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				LowestCost += FMath::Abs(PoseArray[PoseStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
			}

			LowestCost *= CurrentPoseFavour;
		}

		//Todo: Check cost of next natural poses which aren't included in the search loop
		
		//Main Loop Search
		const int32 StartPoseIndex = CurrentMotionData->GetTraitStartIndex(RequiredTraits);
		const int32 EndPoseIndex = CurrentMotionData->GetTraitEndIndex(RequiredTraits);
		for(int32 PoseIndex = StartPoseIndex; PoseIndex < EndPoseIndex; ++PoseIndex)
		{
			float Cost = 0.0f;
			const int32 MatrixStartIndex = PoseIndex * AtomCount;
			const float PoseFavour = PoseArray[MatrixStartIndex]; //Pose cost multiplier is the first atom of a pose array
			
			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[MatrixStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
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

		return MotionData->MatrixPoseIdToDatabasePoseId(LowestPoseId);
	}

	int32 FAnimNode_MSMotionMatching::GetLowestCostPoseId_Linear(const FPoseMotionData& NextPose)
	{
		if (!FinalCalibrationSets.Contains(RequiredTraits))
		{
			return CurrentChosenPoseId;
		}
		
		GenerateCalibrationArray();

		UMotionDataAsset* CurrentMotionData = GetMotionData();
		const int32 AtomCount = CurrentMotionData->SearchPoseMatrix.AtomCount;
		TArray<float>& LookupPoseArray = CurrentMotionData->LookupPoseMatrix.PoseArray;
		
		//Check cost of current pose first for "Favour Current Pose"
		int32 LowestPoseId_LM = 0; //_LM stands for Lookup Matrix, _SM stands for Search Matrix
		float LowestCost = 10000000.0f;
		if(bFavourCurrentPose && !bForcePoseSearch)
		{
			LowestPoseId_LM = CurrentInterpolatedPose.PoseId;
			const int32 PoseStartIndex = LowestPoseId_LM * AtomCount;
			const float PoseFavour = LookupPoseArray[PoseStartIndex]; //Pose cost multiplier is the first atom of a pose array

			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				LowestCost += FMath::Abs(LookupPoseArray[PoseStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
			}

			LowestCost *= CurrentPoseFavour;
		}

		//Next Natural
		LowestPoseId_LM = GetLowestCostNextNaturalId(LowestPoseId_LM, LowestCost, CurrentMotionData); //The returned pose id is in matrix space

		if(bNextNaturalToleranceTest)
		{
			const FPoseMotionData& NextNaturalPose = CurrentMotionData->Poses[LowestPoseId_LM];
			
			if(NextPoseToleranceTest(NextNaturalPose))
			{
				return LowestPoseId_LM;
			}
		}

		int32 LowestPoseId_SM = CurrentMotionData->DatabasePoseIdToMatrixPoseId(LowestPoseId_LM);
		
		//Main Loop Search
		TArray<float>& PoseArray = CurrentMotionData->SearchPoseMatrix.PoseArray;
		const int32 StartPoseIndex = CurrentMotionData->GetTraitStartIndex(RequiredTraits);
		const int32 EndPoseIndex = CurrentMotionData->GetTraitEndIndex(RequiredTraits);
		for(int32 PoseIndex = StartPoseIndex; PoseIndex < EndPoseIndex; ++PoseIndex)
		{
			float Cost = 0.0f;
			const int32 MatrixStartIndex = PoseIndex * AtomCount;
			const float PoseFavour = PoseArray[MatrixStartIndex]; //Pose cost multiplier is the first atom of a pose array
			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(PoseArray[MatrixStartIndex + AtomIndex] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour; 
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId_SM = PoseIndex;
			}
		}

		return CurrentMotionData->MatrixPoseIdToDatabasePoseId(LowestPoseId_SM);
	}

	int32 FAnimNode_MSMotionMatching::GetLowestCostNextNaturalId(int32 LowestPoseId_LM, float LowestCost, UMotionDataAsset* InMotionData)
	{
		//Determine how many valid next naturals there are
		const int32 NextNaturalStart = CurrentInterpolatedPose.PoseId + 1;
		const int32 NextNaturalEnd = CurrentInterpolatedPose.PoseId + (NextNaturalRange / InMotionData->PoseInterval);
		const int32 CurrentAnimId = CurrentInterpolatedPose.AnimId;
		const EMotionAnimAssetType CurrentAnimType = CurrentInterpolatedPose.AnimType;

		int32 ValidNextNaturalCount = 0;
		for(int32 i = NextNaturalStart; i < NextNaturalEnd; ++i)
		{
			const FPoseMotionData& Pose = InMotionData->Poses[i];
			
			if(Pose.SearchFlag > EPoseSearchFlag::NextNatural
				|| Pose.AnimId != CurrentAnimId
				|| Pose.AnimType != CurrentAnimType)
			{
				break;
			}

			++ValidNextNaturalCount;
		}
		
		const int32 AtomCount = InMotionData->SearchPoseMatrix.AtomCount;
		TArray<float>& LookupPoseArray = InMotionData->LookupPoseMatrix.PoseArray;

		const float FinalNextNaturalFavour = bFavourNextNatural ? NextNaturalFavour : 1.0f;

		//Search next naturals and determine the lowest cost one.
		for(int32 PoseIndex = NextNaturalStart; PoseIndex < NextNaturalStart + ValidNextNaturalCount; ++PoseIndex)
		{
			float Cost = 0.0f;
			const int32 MatrixStartIndex = PoseIndex * AtomCount;
			const float PoseFavour = LookupPoseArray[MatrixStartIndex] * FinalNextNaturalFavour;
			for(int32 AtomIndex = 1; AtomIndex < AtomCount; ++AtomIndex)
			{
				Cost += FMath::Abs(LookupPoseArray[MatrixStartIndex + AtomCount] - CurrentInterpolatedPoseArray[AtomIndex])
					* CalibrationArray[AtomIndex - 1] * PoseFavour;
			}

			if(Cost < LowestCost)
			{
				LowestCost = Cost;
				LowestPoseId_LM = PoseIndex;
			}
		}

		return LowestPoseId_LM;
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

	void FAnimNode_MSMotionMatching::JumpToPose(const int32 PoseIdDatabase, const float TimeOffset /*= 0.0f */)
	{
		TimeSinceMotionChosen = TimeSinceMotionUpdate;
		CurrentChosenPoseId = PoseIdDatabase;

		BlendChannels.Empty(TransitionMethod == ETransitionMethod::Blend ? 12 : 1);

		UMotionDataAsset* CurrentMotionData = GetMotionData();
		const FPoseMotionData& Pose = CurrentMotionData->Poses[PoseIdDatabase];

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
		
		//Validate Motion Data
		UMotionDataAsset* CurrentMotionData = GetMotionData();
		if (!CurrentMotionData)
		{
			UE_LOG(LogTemp, Warning, TEXT("Motion matching node failed to initialize. Motion Data has not been set."))
			bValidToEvaluate = false;
			return;
		}

		//Validate Motion Matching Configuration
		UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;
		if (MMConfig)
		{
			MMConfig->Initialize();

			const int32 PoseArraySize = CurrentMotionData->SearchPoseMatrix.AtomCount;

			CurrentInterpolatedPose = FPoseMotionData();
			CurrentInterpolatedPoseArray.Empty(PoseArraySize + 1);
			CalibrationArray.Empty(PoseArraySize + 1);

			CurrentInterpolatedPoseArray.SetNumZeroed(PoseArraySize);
			InputData.DesiredInputArray.SetNumZeroed(PoseArraySize);
			CalibrationArray.SetNumZeroed(PoseArraySize);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Motion matching node failed to initialize. Motion Match Config has not been set on the motion matching node."));
			bValidToEvaluate = false;
			return;
		}

		//Validate MMConfig matches internal calibration (i.e. the MMConfig has not been since changed)
		for(auto& TraitCalibPair : CurrentMotionData->FeatureStandardDeviations)
		{
			FCalibrationData& CalibData = TraitCalibPair.Value;

			if (!CalibData.IsValidWithConfig(MMConfig))
			{
				UE_LOG(LogTemp, Warning, TEXT("Motion matching node failed to initialize. Internal calibration sets atom count does not match the motion config. Did you change the motion config and forget to pre-process?"));
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

		//Apply Standard deviation values to the calibration to allow normalizing of the data
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
			UE_LOG(LogTemp, Warning, TEXT("Motion matching node failed to initialize. Motion Calibration not set in MotionData asset."));
			bValidToEvaluate = false;
			return;
		}

		JumpToPose(0);
		const UAnimSequenceBase* Sequence = GetPrimaryAnim();
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
			UE_LOG(LogTemp, Warning, TEXT("Motion matching node failed to initialize. The starting sequence is null. Check that all animations in the MotionData are valid"));
			bValidToEvaluate = false;
			return;
		}

		MirroringData.Initialize(CurrentMotionData->MirroringProfile, InAnimInstanceProxy->GetSkelMeshComponent());
	}

	bool FAnimNode_MSMotionMatching::NextPoseToleranceTest(const FPoseMotionData& NextPose) const
	{
		if (NextPose.SearchFlag == EPoseSearchFlag::DoNotUse 
		|| NextPose.Traits != RequiredTraits)
		{
			return false;
		}

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		const int32 NextPoseStartIndex = NextPose.PoseId * CurrentMotionData->LookupPoseMatrix.AtomCount;

		int32 FeatureOffset = 1; //Start with offset one because we don't use the pose favour for next pose tolerance test
		for(const TObjectPtr<UMatchFeatureBase> Feature : CurrentMotionData->MotionMatchConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				if(!Feature->NextPoseToleranceTest(InputData.DesiredInputArray,CurrentMotionData->LookupPoseMatrix.PoseArray,
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
		int32 FeatureOffset = 1; //Start at index 1 so as not to apply blending to the pose cost multiplier
		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		for(const TObjectPtr<UMatchFeatureBase> Feature : CurrentMotionData->MotionMatchConfig->Features)
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
		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		for(TObjectPtr<UMatchFeatureBase> FeaturePtr : CurrentMotionData->MotionMatchConfig->Features)
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

		UMotionDataAsset* CurrentMotionData = GetMotionData();
		if(CurrentMotionData
			&& !CurrentMotionData->IsSearchPoseMatrixGenerated())
		{
			CurrentMotionData->GenerateSearchPoseMatrix();
		}
	}

	void FAnimNode_MSMotionMatching::Initialize_AnyThread(const FAnimationInitializeContext& Context)
	{
		DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
		FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);
		
		GetEvaluateGraphExposedInputs().Execute(Context);

		InternalTimeAccumulator = 0.0f;

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		
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
		
		UpdateMotionMatchingState(DeltaTime, Context);

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

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		
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
		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		
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
				AnimSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(static_cast<double>(AnimTime), true));
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
				BlendSpace->GetAnimationPose(PrimaryChannel.BlendSampleDataCache, FAnimExtractContext(static_cast<double>(AnimTime),
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
				Composite->GetAnimationPose(AnimationPoseData, FAnimExtractContext(static_cast<double>(PrimaryChannel.AnimTime), true));
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

			float TotalBlendPower = 0.0f;
			for (int32 i = 0; i < PoseCount; ++i)
			{
				FCompactPose& Pose = ChannelPoses[i];
				FAnimChannelState& AnimChannel = BlendChannels[i];

				const float Weight = AnimChannel.Weight * ((((float)(i + 1)) / ((float)PoseCount)));
				ChannelWeights[i] = Weight;
				TotalBlendPower += Weight;

				float AnimTime = AnimChannel.AnimTime;
				const UMotionDataAsset* CurrentMotionData = GetMotionData();

				switch (AnimChannel.AnimType)
				{
					case EMotionAnimAssetType::Sequence:
					{
						const FMotionAnimSequence& MotionAnim = CurrentMotionData->GetSourceAnimAtIndex(AnimChannel.AnimId);
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
						AnimSequence->GetAnimationPose(AnimationPoseData, FAnimExtractContext(static_cast<double>(AnimTime), true));
					} break;
					case EMotionAnimAssetType::BlendSpace:
					{
						const FMotionBlendSpace& MotionBlendSpace = CurrentMotionData->GetSourceBlendSpaceAtIndex(AnimChannel.AnimId);
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
						BlendSpace->GetAnimationPose(AnimChannel.BlendSampleDataCache, FAnimExtractContext(static_cast<double>(AnimTime),
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
				const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim();

				if(PrimaryAnim)
				{
					FAnimationPoseData AnimationPoseData(Output);
					PrimaryAnim->GetAnimationPose(AnimationPoseData, FAnimExtractContext(
						static_cast<double>(BlendChannels.Last().AnimTime), true));
				}
			}
		}
		else
		{
			const UAnimSequenceBase* PrimaryAnim = GetPrimaryAnim();
			if (PrimaryAnim)
			{
				FAnimationPoseData AnimationPoseData(Output);
				PrimaryAnim->GetAnimationPose(AnimationPoseData, FAnimExtractContext(
					static_cast<double>(BlendChannels.Last().AnimTime), true));
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
		FAnimTickRecord TickRecord(nullptr, true, PlayRate, false,
			FinalBlendWeight, /*inout*/ InternalTimeAccumulator, MarkerTickRecord);
	    
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

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		LowestPoseId = FMath::Clamp(LowestPoseId, 0, CurrentMotionData->Poses.Num() - 1);

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

		//const UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

		// const float TrajectorySearchError = FMath::Abs(ActualChosenTrajectoryCost - LinearChosenTrajectoryCost) / MMConfig->TrajectoryTimes.Num();
		// const float PoseSearchError = FMath::Abs(ActualChosenPoseCost - LinearChosenPoseCost) / MMConfig->PoseBones.Num();

		// const FString OverallMessage = FString::Printf(TEXT("Linear Search Error %f"), PoseSearchError + TrajectorySearchError);
		// const FString PoseMessage = FString::Printf(TEXT("Linear Search Pose Error %f"), PoseSearchError);
		// const FString TrajectoryMessage = FString::Printf(TEXT("Linear Search Trajectory Error %f"), TrajectorySearchError);
		// Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(OverallMessage, FColor::Black);
		// Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(PoseMessage, FColor::Red);
		// Context.AnimInstanceProxy->AnimDrawDebugOnScreenMessage(TrajectoryMessage, FColor::Blue);
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

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		const FAnimChannelState& CurrentChannel = BlendChannels.Last();
		
		switch (CurrentChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence: return CurrentMotionData->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
			case EMotionAnimAssetType::Composite: return CurrentMotionData->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
			default: return nullptr;
		}
	}

	UAnimSequenceBase* FAnimNode_MSMotionMatching::GetPrimaryAnim() const
	{
		if (BlendChannels.Num() == 0)
		{
			return nullptr;
		}

		const UMotionDataAsset* CurrentMotionData = GetMotionData();
		const FAnimChannelState& CurrentChannel = BlendChannels.Last();

		switch (CurrentChannel.AnimType)
		{
			case EMotionAnimAssetType::Sequence: return CurrentMotionData->GetSourceAnimAtIndex(CurrentChannel.AnimId).Sequence;
			case EMotionAnimAssetType::Composite: return CurrentMotionData->GetSourceCompositeAtIndex(CurrentChannel.AnimId).AnimComposite;
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
		
		UMotionMatchConfig* MMConfig = GetMotionData()->MotionMatchConfig;
		
		const FTransform& MeshTransform = InAnimInstanceProxy->GetSkelMeshComponent()->GetComponentTransform();
		const FVector ActorLocation = MeshTransform.GetLocation();
		
		int32 FeatureOffset = 1; //Start at feature offset of 1 for pose cost multiplier
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
		UMotionDataAsset* CurrentMotionData = GetMotionData();
		
		if (InAnimInstanceProxy == nullptr 
		|| CurrentChosenPoseId > CurrentMotionData->Poses.Num() - 1)
		{
			return;
		}

		UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

		int32 FeatureOffset = 1; //Start at feature offset of 1 for pose cost multiplier
		for(const TObjectPtr<UMatchFeatureBase> Feature : MMConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Responsiveness)
			{
				Feature->DrawDebugCurrentRuntime(InAnimInstanceProxy,CurrentMotionData, CurrentInterpolatedPoseArray, FeatureOffset);
			}

			FeatureOffset += Feature->Size();
		}
	}

	void FAnimNode_MSMotionMatching::DrawChosenPoseDebug(FAnimInstanceProxy* InAnimInstanceProxy, bool bDrawVelocity)
	{
		UMotionDataAsset* CurrentMotionData = GetMotionData();
		
		if (InAnimInstanceProxy == nullptr 
		|| CurrentChosenPoseId > CurrentMotionData->Poses.Num() - 1)
		{
			return;
		}

		UMotionMatchConfig* MMConfig = CurrentMotionData->MotionMatchConfig;

		int32 FeatureOffset = 1; //Start at feature offset of 1 for pose cost multiplier
		for(const TObjectPtr<UMatchFeatureBase> Feature : MMConfig->Features)
		{
			if(Feature->PoseCategory == EPoseCategory::Quality)
			{
				Feature->DrawDebugCurrentRuntime(InAnimInstanceProxy,CurrentMotionData, CurrentInterpolatedPoseArray, FeatureOffset);
			}

			FeatureOffset += Feature->Size();
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

		const FPoseMotionData& CurrentPose = CurrentMotionData->Poses[FMath::Clamp(CurrentInterpolatedPose.PoseId,
		0, CurrentMotionData->Poses.Num())];

		
		
		//Print Pose Information
		FString Message = FString::Printf(TEXT("Pose Id: %02d \nPoseFavour: %f \nMirrored: "),
			CurrentPose.PoseId, MotionData->GetPoseFavour(CurrentPose.PoseId));
		
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

		const FMotionAnimAsset* MotionAnimAsset = CurrentMotionData->GetSourceAnim(CurrentPose.AnimId, CurrentPose.AnimType);
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