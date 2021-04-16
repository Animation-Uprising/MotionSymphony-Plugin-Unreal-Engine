// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

//#include "MotionTimelineTrack_Tags.h"
//#include "PersonaUtils.h"
//#include "Animation/AnimSequenceBase.h"
//#include "Widgets/SBoxPanel.h"
//#include "Framework/MultiBox/MultiBoxBuilder.h"
//#include "MotionSequenceTimelineCommands.h"
//#include "Widgets/SMotionTagPanel.h"
//#include "FMotionTimelineTrack_TagsPanel.h"
//#include "MotionTimelineTrack_Tags.h"
//#include "Widgets/Layout/SBorder.h"
//#include "ScopedTransaction.h"
//#include "Animation/AnimMontage.h"
//#include "MotionModel_AnimSequenceBase.h"
//
//#define LOCTEXT_NAMESPACE "FMotionTimelineTrack_Tags"
//
//MOTIONTIMELINE_IMPLEMENT_TRACK(FMotionTimelineTrack_Tags);
//
//FMotionTimelineTrack_Tags::FMotionTimelineTrack_Tags(const TSharedRef<FMotionModel>& InModel)
//	: FMotionTimelineTrack(LOCTEXT("TagsRootTrackLabel", "Tags"), LOCTEXT("TagsRootTrackToolTip", "Tags"), InModel)
//{
//}
//
//TSharedRef<SWidget> FMotionTimelineTrack_Tags::GenerateContainerWidgetForOutliner(const TSharedRef<SMotionOutlinerItem>& InRow)
//{
//	TSharedPtr<SBorder> OuterBorder;
//	TSharedPtr<SHorizontalBox> InnerHorizontalBox;
//	TSharedRef<SWidget> OutlinerWidget = GenerateStandardOutlinerWidget(InRow, true, OuterBorder, InnerHorizontalBox);
//
//	OuterBorder->SetBorderBackgroundColor(FEditorStyle::GetColor("AnimTimeline.Outliner.HeaderColor"));
//
//	UAnimMontage* AnimMontage = Cast<UAnimMontage>(GetModel()->GetAnimSequenceBase());
//	if (!(AnimMontage && AnimMontage->HasParentAsset()))
//	{
//		InnerHorizontalBox->AddSlot()
//			.AutoWidth()
//			.HAlign(HAlign_Right)
//			.VAlign(VAlign_Center)
//			.Padding(OutlinerRightPadding, 1.0f)
//			[
//				PersonaUtils::MakeTrackButton(LOCTEXT("AddTrackButtonText", "Track"), FOnGetContent::CreateSP(this, &FMotionTimelineTrack_Tags::BuildNotifiesSubMenu), MakeAttributeSP(this, &FMotionTimelineTrack_Tags::IsHovered))
//			];
//	}
//
//	return OutlinerWidget;
//}
//
//TSharedRef<SWidget> FMotionTimelineTrack_Tags::BuildNotifiesSubMenu()
//{
//	FMenuBuilder MenuBuilder(true, GetModel()->GetCommandList());
//
//	MenuBuilder.BeginSection("Tags", LOCTEXT("TagsMenuSection", "Tags"));
//	{
//		MenuBuilder.AddMenuEntry(
//			FMotionSequenceTimelineCommands::Get().AddNotifyTrack->GetLabel(),
//			FMotionSequenceTimelineCommands::Get().AddNotifyTrack->GetDescription(),
//			FMotionSequenceTimelineCommands::Get().AddNotifyTrack->GetIcon(),
//			FUIAction(
//				FExecuteAction::CreateSP(this, &FMotionTimelineTrack_Tags::AddTrack)
//			)
//		);
//	}
//	MenuBuilder.EndSection();
//
//	MenuBuilder.BeginSection("TimingPanelOptions", LOCTEXT("TimingPanelOptionsHeader", "Options"));
//	{
//		MenuBuilder.AddMenuEntry(
//			LOCTEXT("ToggleTimingNodes_Tags", "Show Tag Timing Nodes"),
//			LOCTEXT("ShowTagTimingNodes", "Show or hide the timing display for Tags in the tag panel"),
//			FSlateIcon(),
//			FUIAction(
//				FExecuteAction::CreateSP(&StaticCastSharedRef<FMotionModel_AnimSequenceBase>(GetModel()).Get(), &FMotionModel_AnimSequenceBase::ToggleNotifiesTimingElementDisplayEnabled, ETimingElementType::QueuedNotify),
//				FCanExecuteAction(),
//				FIsActionChecked::CreateSP(&StaticCastSharedRef<FMotionModel_AnimSequenceBase>(GetModel()).Get(), &FMotionModel_AnimSequenceBase::IsNotifiesTimingElementDisplayEnabled, ETimingElementType::QueuedNotify)
//			),
//			NAME_None,
//			EUserInterfaceActionType::ToggleButton
//		);
//	}
//	MenuBuilder.EndSection();
//
//	return MenuBuilder.MakeWidget();
//}
//
//void FMotionTimelineTrack_Tags::AddTrack()
//{
//	UAnimSequenceBase* AnimSequenceBase = GetModel()->GetAnimSequenceBase();
//
//	FScopedTransaction Transaction(LOCTEXT("AddTagTrack", "Add Tag Track"));
//	AnimSequenceBase->Modify();
//
//	FAnimNotifyTrack NewItem;
//	NewItem.TrackName = GetNewTrackName(AnimSequenceBase);
//	NewItem.TrackColor = FLinearColor::White;
//
//	AnimSequenceBase->AnimNotifyTracks.Add(NewItem);
//
//	TagsPanel.Pin()->RequestTrackRename(AnimSequenceBase->AnimNotifyTracks.Num() - 1);
//	TagsPanel.Pin()->Update();
//}
//
//FName FMotionTimelineTrack_Tags::GetNewTrackName(UAnimSequenceBase* InAnimSequenceBase)
//{
//	TArray<FName> TrackNames;
//	TrackNames.Reserve(50);
//
//	for (const FAnimNotifyTrack& Track : InAnimSequenceBase->AnimNotifyTracks)
//	{
//		TrackNames.Add(Track.TrackName);
//	}
//
//	FName NameToTest;
//	int32 TrackIndex = 1;
//
//	do
//	{
//		NameToTest = *FString::FromInt(TrackIndex++);
//	} while (TrackNames.Contains(NameToTest));
//
//	return NameToTest;
//}
//
//#undef LOCTEXT_NAMESPACE