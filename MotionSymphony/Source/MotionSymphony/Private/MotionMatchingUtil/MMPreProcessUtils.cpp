// Copyright 2020 Kenneth Claassen. All Rights Reserved.


#include "MMPreProcessUtils.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimationBlueprintLibrary.h"
#include "Enumerations/ETrajectoryPreProcessMethod.h"

void FMMPreProcessUtils::ExtractRootVelocity(FVector& o_rootVelocity,
	const UAnimSequence* a_anim, const float a_time, const float a_poseInterval)
{
	float startTime = a_time - (a_poseInterval / 2.0f);

	FVector rootDelta = a_anim->ExtractRootMotion(startTime, a_poseInterval, false).GetTranslation();

	o_rootVelocity = rootDelta.GetSafeNormal() * (rootDelta.Size() / a_poseInterval);
}

void FMMPreProcessUtils::ExtractPastTrajectoryPoint(FTrajectoryPoint& outTrajPoint, 
	const UAnimSequence* AnimSequence, const float BaseTime, const float PointTime, 
	ETrajectoryPreProcessMethod PastMethod, UAnimSequence* PrecedingMotion)
{
	float pointAnimTime = BaseTime + PointTime;

	//Root delta to the beginning of the clip
	FTransform rootDelta;

	if ((int)PastMethod > (int)ETrajectoryPreProcessMethod::IgnoreEdges 
		&& pointAnimTime < 0.0f)
	{
		//Trajectory point time is outside the bounds of the clip and we are not ignoring edges
		rootDelta = AnimSequence->ExtractRootMotion(BaseTime, -BaseTime, false);
		
		switch (PastMethod)
		{
			//Extrapolate the motion at the beginning of the clip
			case ETrajectoryPreProcessMethod::Extrapolate:
			{
				FTransform initialMotion = AnimSequence->ExtractRootMotion(0.05f, -0.05f, false);

				//transform the root delta by initial motion for a required number of iterations
				int iterations = FMath::RoundToInt(FMath::Abs(pointAnimTime) / 0.05f);
				for (int i = 0; i < iterations; ++i)
				{
					rootDelta *= initialMotion;
				}

			} break;

			case ETrajectoryPreProcessMethod::Animation:
			{
				if (PrecedingMotion == nullptr)
					break;

				FTransform precedingRootDelta = PrecedingMotion->ExtractRootMotion(PrecedingMotion->SequenceLength, pointAnimTime, false);

				rootDelta *= precedingRootDelta;

			} break;
		}
	}
	else
	{
		//Here the trajectory point either falls within the clip or we are ignoring edges
		//therefore, no fanciness is required
		float time = FMath::Clamp(PointTime, -BaseTime, 0.0f);

		rootDelta = AnimSequence->ExtractRootMotion(BaseTime, time, false);
	}

	//Apply the calculated root deltas
	outTrajPoint = FTrajectoryPoint();
	outTrajPoint.Position = rootDelta.GetTranslation();
	outTrajPoint.RotationZ = rootDelta.GetRotation().Euler().Z;
}

void FMMPreProcessUtils::ExtractFutureTrajectoryPoint(FTrajectoryPoint& outTrajPoint, 
	const UAnimSequence* AnimSequence, const float BaseTime, const float PointTime, 
	ETrajectoryPreProcessMethod FutureMethod, UAnimSequence* FollowingMotion)
{
	float pointAnimTime = BaseTime + PointTime;

	//Root delta to the beginning of the clip
	FTransform rootDelta;

	if ((int)FutureMethod > (int)ETrajectoryPreProcessMethod::IgnoreEdges
		&& pointAnimTime > AnimSequence->SequenceLength)
	{
		//Trajectory point time is outside the bounds of the clip and we are not ignoring edges

		rootDelta = AnimSequence->ExtractRootMotion(BaseTime, AnimSequence->SequenceLength - BaseTime, false);

		switch (FutureMethod)
		{
			//Extrapolate the motion at the end of the clip
			case ETrajectoryPreProcessMethod::Extrapolate:
			{
				FTransform endMotion = AnimSequence->ExtractRootMotion(AnimSequence->SequenceLength - 0.05f, 0.05f, false);

				//transform the root delta by initial motion for a required number of iterations
				int iterations = FMath::RoundToInt(FMath::Abs(pointAnimTime) / 0.05f);
				for (int i = 0; i < iterations; ++i)
				{
					rootDelta *= endMotion;
				}

			} break;

			case ETrajectoryPreProcessMethod::Animation:
			{
				if (FollowingMotion == nullptr)
					break;

				FTransform followingRootDelta = FollowingMotion->ExtractRootMotion(0.0f, pointAnimTime - AnimSequence->SequenceLength, false);

				rootDelta *= followingRootDelta;

			} break;
		}
	}
	else
	{
		//Here the trajectory point either falls within the clip or we are ignoring edges
		//therefore, no fanciness is required
		float time = FMath::Clamp(PointTime, 0.0f, AnimSequence->SequenceLength - BaseTime);

		rootDelta = AnimSequence->ExtractRootMotion(BaseTime, time, false);
	}

	//Apply the calculated root deltas
	outTrajPoint = FTrajectoryPoint();
	outTrajPoint.Position = rootDelta.GetTranslation();
	outTrajPoint.RotationZ = rootDelta.GetRotation().Euler().Z;
}

void FMMPreProcessUtils::ExtractLoopingTrajectoryPoint(FTrajectoryPoint& outTrajPoint, 
	const UAnimSequence* AnimSequence, const float BaseTime, const float PointTime)
{
	FTransform rootDelta = AnimSequence->ExtractRootMotion(BaseTime, PointTime, true);

	outTrajPoint = FTrajectoryPoint();
	outTrajPoint.Position = rootDelta.GetTranslation();
	outTrajPoint.RotationZ = rootDelta.GetRotation().Euler().Z;
}

void FMMPreProcessUtils::ExtractJointData(FJointData& OutJointData, 
	const UAnimSequence* Anim, const int JointId, const float Time, const float PoseInterval)
{
	//FReferenceSkeleton RefSkeleton = Anim->GetSkeleton()->GetReferenceSkeleton();

	FTransform JointTransform = FTransform::Identity;
	GetJointTransform_RootRelative(JointTransform, Anim, JointId, Time);

	FVector JointVelocity = FVector::ZeroVector;
	GetJointVelocity_RootRelative(JointVelocity, Anim, JointId, Time, PoseInterval);

	OutJointData = FJointData(JointTransform.GetLocation(), JointVelocity);
}

void FMMPreProcessUtils::ExtractJointData(FJointData& OutJointData, const UAnimSequence* Anim,
	 const FBoneReference& BoneReference, const float Time, const float PoseInterval)
{
	TArray<FName> BonesToRoot;
	UAnimationBlueprintLibrary::FindBonePathToRoot(Anim, BoneReference.BoneName, BonesToRoot);
	BonesToRoot.RemoveAt(BonesToRoot.Num() - 1); 

	FTransform JointTransform_CS = FTransform::Identity;
	GetJointTransform_RootRelative(JointTransform_CS, Anim, BonesToRoot, Time);

	FVector JointVelocity_CS = FVector::ZeroVector;
	GetJointVelocity_RootRelative(JointVelocity_CS, Anim, BonesToRoot, Time, PoseInterval);

	OutJointData = FJointData(JointTransform_CS.GetLocation(), JointVelocity_CS);
}

void FMMPreProcessUtils::GetJointVelocity_RootRelative(FVector & OutJointVelocity, 
	const UAnimSequence * Anim, const int JointId, const float Time, const float PoseInterval)
{
	float StartTime = Time - (PoseInterval / 2.0f);

	FTransform BeforeTransform = FTransform::Identity;
	GetJointTransform_RootRelative(BeforeTransform, Anim, JointId, StartTime);

	FTransform AfterTransform = FTransform::Identity;
	GetJointTransform_RootRelative(AfterTransform, Anim, JointId, StartTime + PoseInterval);

	OutJointVelocity = (AfterTransform.GetLocation() - BeforeTransform.GetLocation()) / PoseInterval;
}

void FMMPreProcessUtils::GetJointVelocity_RootRelative(FVector& OutVelocity, const UAnimSequence* AniMSequence, 
	const TArray<FName>& BonesToRoot, const float Time, const float PoseInterval)
{
	float StartTime = Time - (PoseInterval / 2.0f);

	FTransform BeforeTransform = FTransform::Identity;
	GetJointTransform_RootRelative(BeforeTransform, AniMSequence, BonesToRoot, StartTime);

	FTransform AfterTransform = FTransform::Identity;
	GetJointTransform_RootRelative(AfterTransform, AniMSequence, BonesToRoot, StartTime + PoseInterval);

	OutVelocity = (AfterTransform.GetLocation() - BeforeTransform.GetLocation()) / PoseInterval;
}

bool FMMPreProcessUtils::GetDoNotUseTag(const UAnimSequence * Anim, const float Time, const float PoseInterval)
{
	float StartTime = Time - (PoseInterval / 2.0f);

	TArray<FAnimNotifyEventReference> Notifies;
	Anim->GetAnimNotifies(StartTime, PoseInterval, false, Notifies);

	for (int32 i = 0; i < Notifies.Num(); ++i)
	{
		UAnimNotifyState* NotifyState = Notifies[i].GetNotify()->NotifyStateClass;
	
		if (NotifyState)
		{
			if (NotifyState->GetNotifyName() == "MSDoNotUse")
			{
				return true;
			}
		}
	}

	return false;
}

void FMMPreProcessUtils::GetJointTransform_RootRelative(FTransform & OutJointTransform,
	const UAnimSequence * Anim, const int JointId, const float Time)
{
	OutJointTransform = FTransform::Identity;

	if (Anim && JointId != INDEX_NONE)
	{
		FReferenceSkeleton RefSkeleton = Anim->GetSkeleton()->GetReferenceSkeleton();

		if (RefSkeleton.IsValidIndex(JointId))
		{
			int32 ConvertedJointId = Anim->GetAnimationTrackNames().IndexOfByKey(RefSkeleton.GetBoneName(JointId));
			Anim->GetBoneTransform(OutJointTransform, ConvertedJointId, Time, true);

			int32 CurrentJointId = JointId;

			if (CurrentJointId == 0)
			{
				return;
			}

			while (RefSkeleton.GetRawParentIndex(CurrentJointId) != 0)
			{
				//Need to get parents by name
				int32 ParentJointId = RefSkeleton.GetRawParentIndex(CurrentJointId);
				ConvertedJointId = Anim->GetAnimationTrackNames().IndexOfByKey(RefSkeleton.GetBoneName(ParentJointId));

				FTransform ParentTransform;
				Anim->GetBoneTransform(ParentTransform, ParentJointId, Time, true);

				OutJointTransform = OutJointTransform * ParentTransform;
				CurrentJointId = ParentJointId;
			}
		}
		else
		{
			OutJointTransform = FTransform::Identity;
		}
	}
}

void FMMPreProcessUtils::GetJointTransform_RootRelative(FTransform& OutTransform, 
	const UAnimSequence* AnimSequence, const TArray<FName>& BonesToRoot, const float Time)
{
	OutTransform = FTransform::Identity;
	for (const FName& BoneName : BonesToRoot)
	{
		FTransform BoneTransform;
		int32 ConvertedBoneIndex = AnimSequence->GetAnimationTrackNames().IndexOfByKey(BoneName);
		AnimSequence->GetBoneTransform(BoneTransform, ConvertedBoneIndex, Time, true);

		OutTransform = OutTransform * BoneTransform;
	}
}
