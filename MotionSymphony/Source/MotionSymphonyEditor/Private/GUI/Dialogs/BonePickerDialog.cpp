// Copyright 2020 Kenneth Claassen. All Rights Reserved.

#include "BonePickerDialog.h"
#include "MotionPreProcessToolkit.h"

#include "CoreMinimal.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "MotionSymphonyEditor"

void SBonePickerDialog::Construct(const FArguments& InArgs, TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	MotionPreProcessToolkitPtr = InMotionPreProcessToolkitPtr;

	TSharedPtr<SVerticalBox> MainBox = SNew(SVerticalBox);

	//Get a list of all bones in the skeleton by name
	USkeleton* skeleton = MotionPreProcessToolkitPtr.Get()->GetSkeleton();
	FReferenceSkeleton refSkeleton = skeleton->GetReferenceSkeleton();

	for (int i = 0; i < refSkeleton.GetNum(); ++i)
	{
		m_boneNames.Add(FText::FromName(refSkeleton.GetBoneName(i)));

		TSharedPtr<SCheckBox> checkBox = SNew(SCheckBox);
		m_checkBoxes.Add(checkBox);
	}

	int index = 0;
	for (auto& boneName : m_boneNames)
	{
		MainBox->AddSlot()
			.FillHeight(0.5f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					m_checkBoxes[index].ToSharedRef()
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.Text(boneName)
				]
			];

		++index;
	}

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
			.Padding(FMargin(1.0f, 1.0f, 1.0f, 1.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(1.0f)
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						MainBox.ToSharedRef()
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SUniformGridPanel)
						.SlotPadding(1)
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SButton)
							.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
							.ForegroundColor(FLinearColor::White)
							.Text(LOCTEXT("ConfirmButton", "Confirm"))
							.OnClicked(this, &SBonePickerDialog::OnConfirm)
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SButton)
							.ButtonStyle(FEditorStyle::Get(), "FlatButton")
							.ForegroundColor(FLinearColor::White)
							.Text(LOCTEXT("CancelButton", "Cancel"))
							.OnClicked(this, &SBonePickerDialog::OnCancel)
						]
					]
				]
			]
		];
}

SBonePickerDialog::~SBonePickerDialog()
{
}

bool SBonePickerDialog::ShowWindow(TSharedPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
{
	const FText TitleText = NSLOCTEXT("MotionPreProcessToolkit", "MotionPreProcessToolkit_BonePicker", "Bone Picker");
	TSharedRef<SWindow> BonePickerWindow = SNew(SWindow)
		.Title(TitleText)
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(550.0f, 1000.0f))
		.AutoCenter(EAutoCenter::PreferredWorkArea)
		.SupportsMinimize(false);

	TSharedRef<SBonePickerDialog> BonePickerDialog = SNew(SBonePickerDialog, InMotionPreProcessToolkitPtr);

	BonePickerWindow->SetContent(BonePickerDialog);
	TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	if (RootWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(BonePickerWindow, RootWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(BonePickerWindow);
	}

	return false;
}

FReply SBonePickerDialog::OnConfirm()
{
	MotionPreProcessToolkitPtr->ClearMatchBones();

	for (int i = 0; i < m_checkBoxes.Num(); ++i)
	{
		if (m_checkBoxes[i]->IsChecked())
		{
			MotionPreProcessToolkitPtr->AddMatchBone(i);
		}
	}


	CloseContainingWindow();

	return FReply::Handled();
}

FReply SBonePickerDialog::OnCancel()
{
	CloseContainingWindow();
	return FReply::Handled();
}

void SBonePickerDialog::CloseContainingWindow()
{
	TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	if (ContainingWindow.IsValid())
	{
		ContainingWindow->RequestDestroyWindow();
	}
}

#undef LOCTEXT_NAMESPACE