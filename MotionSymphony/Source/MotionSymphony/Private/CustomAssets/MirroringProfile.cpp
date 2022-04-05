// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.


#include "CustomAssets/MirroringProfile.h"

#define LOCTEXT_NAMESPACE "MirroringProfile"

FBoneMirrorPair::FBoneMirrorPair()
	: BoneName(),
	  MirrorBoneName(),
	  MirrorAxis(EAxis::Y),
	  FlipAxis(EAxis::X),
	  bHasMirrorBone(false),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

FBoneMirrorPair::FBoneMirrorPair(const FString& InBoneName, const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis)
	: BoneName(InBoneName),
	  MirrorBoneName(InBoneName),
	  MirrorAxis(InMirrorAxis),
	  FlipAxis(InFlipAxis),
	  bHasMirrorBone(false),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

FBoneMirrorPair::FBoneMirrorPair(const FString& InBoneName, const FString& InMirrorBoneName, 
	const EAxis::Type InMirrorAxis, const EAxis::Type InFlipAxis)
	: BoneName(InBoneName),
	  MirrorBoneName(InMirrorBoneName),
	  MirrorAxis(InMirrorAxis),
	  FlipAxis(InFlipAxis),
	  bHasMirrorBone(true),
	  bMirrorPosition(false),
	  RotationOffset(FRotator::ZeroRotator)
{
}

UMirroringProfile::UMirroringProfile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	SourceSkeleton(nullptr),
	CharacterMirrorAxis(FVector::ForwardVector),
	bMirrorPosition_Default(false),
	LeftAffix("_l"),
	RightAffix("_r")
{
}

void UMirroringProfile::AutoMap()
{
	if (SourceSkeleton == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot auto map mirroring profile with a null skeleton"));
		return;
	}

	const FReferenceSkeleton& RefSkeleton = SourceSkeleton->GetReferenceSkeleton();
	int32 BoneCount = RefSkeleton.GetNum();
	MirrorPairs.Empty(BoneCount + 1);
	TArray<FString> BoneStrings;

	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
	{
		FString BoneStr = RefSkeleton.GetBoneName(BoneIndex).ToString();

		int32 LeftAffixPosition = BoneStr.Find(LeftAffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd, -1);
		int32 RightAffixPosition = BoneStr.Find(RightAffix, ESearchCase::IgnoreCase, ESearchDir::FromEnd, -1);

		if (LeftAffixPosition != -1 || RightAffixPosition != -1)
		{
			FString MirrorBoneStr;
			bool ValidMirror = true;
			if (RightAffixPosition != -1)
			{
				//Construct the left affix mirror bone name
				MirrorBoneStr = BoneStr.Mid(0, RightAffixPosition);
				MirrorBoneStr += LeftAffix;
				MirrorBoneStr += BoneStr.Mid(RightAffixPosition + RightAffix.Len());

				//Make sure the mirror bone is a valid bone in the skeleton
				if (RefSkeleton.FindBoneIndex(FName(MirrorBoneStr)) == INDEX_NONE
				|| FMath::Abs(MirrorBoneStr.Len() - LeftAffix.Len()) != FMath::Abs(BoneStr.Len() - LeftAffix.Len()))
				{
					ValidMirror = false;
				}
			}
			else
			{
				//Construct the right affix mirror bone name
				MirrorBoneStr = BoneStr.Mid(0, LeftAffixPosition);
				MirrorBoneStr += RightAffix;
				MirrorBoneStr += BoneStr.Mid(LeftAffixPosition + LeftAffix.Len());

				//Make sure the mirror bone is a valid bone in the skeleton
				if (RefSkeleton.FindBoneIndex(FName(MirrorBoneStr)) == INDEX_NONE
				|| FMath::Abs(MirrorBoneStr.Len() - RightAffix.Len()) != FMath::Abs(BoneStr.Len() - RightAffix.Len()))
				{
					ValidMirror = false;
				}
			}

			//Provided there is a valid mirror pair, add a mirror pair to the mirroring profile
			if (ValidMirror)
			{
				FName MirrorBoneName = FName(*MirrorBoneStr);
				int32 MirrorBoneIndex = RefSkeleton.FindBoneIndex(FName(*MirrorBoneStr));

				if (!BoneStrings.Contains(MirrorBoneStr) && !BoneStrings.Contains(BoneStr))
				{
					FBoneMirrorPair NewMirrorPair = FBoneMirrorPair(BoneStr, MirrorBoneStr, EAxis::None, EAxis::None);
					NewMirrorPair.bMirrorPosition = bMirrorPosition_Default;

					//Calculate RotationOffset
					const TArray<FTransform>& RefBonePose = RefSkeleton.GetRefBonePose();
					NewMirrorPair.RotationOffset = RefBonePose[BoneIndex].Rotator() - RefBonePose[MirrorBoneIndex].Rotator();

					MirrorPairs.Add(NewMirrorPair);
					BoneStrings.Add(MirrorBoneStr);
					BoneStrings.Add(BoneStr);
					continue;
				}
			}
		}

		//If a mirror valid mirror pair wasn't found then we can setup an individual bone mirror profile
		if (!BoneStrings.Contains(BoneStr))
		{
			FBoneMirrorPair NewMirrorPair = FBoneMirrorPair(BoneStr, EAxis::None, EAxis::None);
			NewMirrorPair.RotationOffset = FRotator(0.0f, 0.0f, 0.0f);
			NewMirrorPair.bMirrorPosition = bMirrorPosition_Default;

			NewMirrorPair.MirrorAxis = GetMirrorAxis(BoneIndex);
			NewMirrorPair.FlipAxis = GetFlipAxis(BoneIndex);

			MirrorPairs.Add(NewMirrorPair);
			BoneStrings.Add(BoneStr);
		}
	}
}

FName UMirroringProfile::FindBoneMirror(FName BoneName)
{
	FString BoneStr = BoneName.ToString();

	for (FBoneMirrorPair& MirrorPair : MirrorPairs)
	{
		if (MirrorPair.BoneName == BoneStr)
		{
			if (MirrorPair.bHasMirrorBone)
			{
				return FName(MirrorPair.MirrorBoneName);
			}
			else
			{
				return BoneName;
			}
		}
		else if (MirrorPair.MirrorBoneName == BoneStr)
		{
			if (MirrorPair.bHasMirrorBone)
			{
				return FName(MirrorPair.BoneName);
			}
			else
			{
				return BoneName;
			}
		}
	}

	return BoneName;
}

TEnumAsByte<EAxis::Type> UMirroringProfile::GetMirrorAxis(int32 BoneIndex)
{
	if (BoneIndex < 0)
	{
		return EAxis::None;
	}

	const FReferenceSkeleton& RefSkeleton = SourceSkeleton->GetReferenceSkeleton();
	if (BoneIndex >= RefSkeleton.GetNum())
	{
		return EAxis::None;
	}

	const TArray<FTransform>& RefBonePose = RefSkeleton.GetRefBonePose();

	//Find Mirror Axis
	FTransform BoneTransform = RefBonePose[BoneIndex];
	int32 ParentBoneIndex = RefSkeleton.GetParentIndex(BoneIndex);

	if (ParentBoneIndex < 1)
	{
		BoneTransform = RefBonePose[0];
	}
	else
	{
		while (ParentBoneIndex > 0)
		{
			FTransform ParentTransform = RefBonePose[ParentBoneIndex];
			BoneTransform = BoneTransform * ParentTransform;

			ParentBoneIndex = RefSkeleton.GetParentIndex(ParentBoneIndex);
		}
	}

	TEnumAsByte<EAxis::Type> BestAxis = EAxis::X;
	FVector BoneXAxis = BoneTransform.GetUnitAxis(EAxis::X);
	float LowestCostAxis = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneXAxis));

	FVector BoneYAxis = BoneTransform.GetUnitAxis(EAxis::Y);
	float YAxisCost = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneYAxis));

	if (YAxisCost < LowestCostAxis)
	{
		LowestCostAxis = YAxisCost;
		BestAxis = EAxis::Y;
	}

	FVector BoneZAxis = BoneTransform.GetUnitAxis(EAxis::Z);
	float ZAxisCost = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneZAxis));

	if (ZAxisCost < LowestCostAxis)
	{
		BestAxis = EAxis::Z;
	}

	return BestAxis;
}

TEnumAsByte<EAxis::Type> UMirroringProfile::GetFlipAxis(int32 BoneIndex)
{
	if (BoneIndex < 0)
	{
		return EAxis::None;
	}

	const FReferenceSkeleton& RefSkeleton = SourceSkeleton->GetReferenceSkeleton();

	if (BoneIndex >= RefSkeleton.GetNum())
	{
		return EAxis::None;
	}

	const TArray<FTransform>& RefBonePose = RefSkeleton.GetRefBonePose();

	FTransform BoneTransform = RefBonePose[BoneIndex];
	int32 ParentBoneIndex = RefSkeleton.GetParentIndex(BoneIndex);
	while (ParentBoneIndex != INDEX_NONE)
	{
		FTransform ParentTransform = RefBonePose[ParentBoneIndex];
		BoneTransform = BoneTransform * ParentTransform;

		ParentBoneIndex = RefSkeleton.GetParentIndex(ParentBoneIndex);
	}

	TEnumAsByte<EAxis::Type> BestAxis = EAxis::X;
	FVector BoneXAxis = BoneTransform.GetUnitAxis(EAxis::X);
	float LowestCostAxis = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneXAxis));

	FVector BoneYAxis = BoneTransform.GetUnitAxis(EAxis::Y);
	float YAxisCost = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneYAxis));

	if (YAxisCost < LowestCostAxis)
	{
		LowestCostAxis = YAxisCost;
		BestAxis = EAxis::Y;
	}

	FVector BoneZAxis = BoneTransform.GetUnitAxis(EAxis::Z);
	float ZAxisCost = 1 - FMath::Abs(FVector::DotProduct(CharacterMirrorAxis, BoneZAxis));

	if (ZAxisCost < LowestCostAxis)
	{
		BestAxis = EAxis::Z;
	}

	return BestAxis;
}

USkeleton* UMirroringProfile::GetSourceSkeleton()
{
	return SourceSkeleton;
}

void UMirroringProfile::SetSourceSkeleton(USkeleton* skeleton)
{
	Modify();
	SourceSkeleton = skeleton;
	MarkPackageDirty();
}

bool UMirroringProfile::IsSetupValid()
{
	if (!SourceSkeleton)
	{
		return false;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

