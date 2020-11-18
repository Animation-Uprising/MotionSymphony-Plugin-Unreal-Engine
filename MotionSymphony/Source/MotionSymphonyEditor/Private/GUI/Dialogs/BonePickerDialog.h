// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWidget.h"

class SBonePickerDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBonePickerDialog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<class FMotionPreProcessToolkit> InMotionPreProcessTookitPtr);

	~SBonePickerDialog();

	static bool ShowWindow(TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr);

private:
	FReply OnConfirm();
	FReply OnCancel();
	void CloseContainingWindow();
	TSharedPtr<class IDetailView> MainPropertyView;
	TSharedPtr<class FMotionPreProcessToolkit> MotionPreProcessToolkitPtr;

	TArray<FText> m_boneNames;
	TArray<TSharedPtr<SCheckBox>> m_checkBoxes;
};