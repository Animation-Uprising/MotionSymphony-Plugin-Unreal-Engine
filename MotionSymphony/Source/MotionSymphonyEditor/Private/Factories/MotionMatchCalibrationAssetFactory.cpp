// Copyright 2021 Kenneth Claassen. All Rights Reserved.

#include "MotionMatchCalibrationAssetFactory.h"
#include "CustomAssets/MotionCalibration.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "MotionMatchCalibrationFactory"

UMotionCalibrationFactory::UMotionCalibrationFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UMotionCalibration::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMotionCalibrationFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	UMotionCalibration* NewMMCalibration = NewObject<UMotionCalibration>(InParent, InClass, InName, Flags);

	if (FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("Select Skeleton for Motion Matching Calibration",
		"Motion matching configurations need to be paired with a skeleton to function properly. Do you want to choose one now?"))
		== EAppReturnType::Yes)
	{
		//SSkeletonPickerDialog::OnOpenFollowUpWindow.BindSP(NewMMCalibration, &UMotionCalibration::OpenPickBonesDialog);
		//SSkeletonPickerDialog::ShowWindow(NewMMCalibration);
	}

	return NewMMCalibration;
}

bool UMotionCalibrationFactory::ShouldShowInNewMenu() const
{
	return true;
}

#undef LOCTEXXT_NAMESPACE