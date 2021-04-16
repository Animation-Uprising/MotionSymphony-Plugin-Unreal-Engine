// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

//#include "FMotionTimelineTrack_TagsPanel.h"
//#include "Widgets/SMotionTagPanel.h"
//#include "Framework/MultiBox/MultiBoxBuilder.h"
//#include "Widgets/Layout/SBox.h"
//#include "PersonaUtils.h"
//#include "MotionSequenceTimelineCommands.h"
//#include "Widgets/Text/SInlineEditableTextBlock.h"
//#include "MotionTimelineTrack_Tags.h"
//#include "ScopedTransaction.h"
//#include "Widgets/Views/SExpanderArrow.h"
//#include "Widgets/Layout/SBorder.h"
//#include "Widgets/SMotionOutlinerItem.h"
//#include "Animation/AnimMontage.h"
//#include "MotionModel_AnimSequenceBase.h"
//
//#define LOCTEXT_NAMESPACE "FMotionTimelineTrack_TagsPanel"
//
//const float FMotionTimelineTrack_TagsPanel::NotificationTrackHeight = 24.0f;
//
//MOTIONTIMELINE_IMPLEMENT_TRACK(FMotionTimelineTrack_TagsPanel);
//
//FMotionTimelineTrack_TagsPanel::FMotionTimelineTrack_TagsPanel(const TSharedRef<FMotionModel>& InModel)
//	: FMotionTimelineTrack(FText::GetEmpty(), FText::GetEmpty(), InModel)
//	, PendingRenameTrackIndex(INDEX_NONE)
//{
//	SetHeight((float)InModel->GetAnimSequenceBase()->AnimNotifyTracks.Num() * NotificationTrackHeight);
//
//	//SetHeight((float)InModel->MotionAnim->)
//}
//
//TSharedRef<SWidget> FMotionTimelineTrack_TagsPanel::GenerateContainerWidgetForTimeline()
//{
//	GetAnimNotifyPanel();
//
//	MotionTagPanel->Update();
//
//	return MotionTagPanel.ToSharedRef();
//}
//
//TSharedRef<SWidget> FMotionTimelineTrack_TagsPanel::GenerateContainerWidgetForOutliner(const TSharedRef<SMotionOutlinerItem>& InRow)
//{
//	TSharedRef<SWidget> Widget =
//		SNew(SHorizontalBox)
//		.ToolTipText(this, &FMotionTimelineTrack::GetTooltipText)
//		+ SHorizontalBox::Slot()
//		[
//			SAssignNew(OutlinerWidget, SVerticalBox)
//		];
//
//	RefreshOutlinerWidget();
//
//	return Widget;
//}
//
//void FMotionTimelineTrack_TagsPanel::RefreshOutlinerWidget()
//{
//	OutlinerWidget->ClearChildren();
//
//	int32 TrackIndex = 0;
//	UAnimSequenceBase* AnimSequence = GetModel()->GetAnimSequenceBase();
//	for (FAnimNotifyTrack& AnimNotifyTrack : AnimSequence->AnimNotifyTracks)
//	{
//		TSharedPtr<SBox> SlotBox;
//		TSharedPtr<SInlineEditableTextBlock> InlineEditableTextBlock;
//
//		OutlinerWidget->AddSlot()
//			.AutoHeight()
//			[
//				SAssignNew(SlotBox, SBox)
//				.HeightOverride(NotificationTrackHeight)
//			];
//
//		TSharedPtr<SHorizontalBox> HorizontalBox;
//
//		SlotBox->SetContent(
//			SNew(SBorder)
//			.BorderImage(FEditorStyle::GetBrush("Sequencer.Section.BackgroundTint"))
//			.BorderBackgroundColor(FEditorStyle::GetColor("AnimTimeline.Outliner.ItemColor"))
//			[
//				SAssignNew(HorizontalBox, SHorizontalBox)
//				+ SHorizontalBox::Slot()
//			.FillWidth(1.0f)
//			.VAlign(VAlign_Center)
//			.HAlign(HAlign_Left)
//			.Padding(30.0f, 0.0f, 0.0f, 0.0f)
//			[
//				SAssignNew(InlineEditableTextBlock, SInlineEditableTextBlock)
//				.Text_Lambda([TrackIndex, AnimSequence]() { return AnimSequence->AnimNotifyTracks.IsValidIndex(TrackIndex) ? FText::FromName(AnimSequence->AnimNotifyTracks[TrackIndex].TrackName) : FText::GetEmpty(); })
//			.IsSelected(FIsSelected::CreateLambda([]() { return true; }))
//			.OnTextCommitted(this, &FMotionTimelineTrack_TagsPanel::OnCommitTrackName, TrackIndex)
//			]
//
//			]
//		);
//
//		UAnimMontage* AnimMontage = Cast<UAnimMontage>(GetModel()->GetAnimSequenceBase());
//		if (!(AnimMontage && AnimMontage->HasParentAsset()))
//		{
//			HorizontalBox->AddSlot()
//				.AutoWidth()
//				.VAlign(VAlign_Center)
//				.HAlign(HAlign_Right)
//				.Padding(OutlinerRightPadding, 1.0f)
//				[
//					PersonaUtils::MakeTrackButton(LOCTEXT("AddTrackButtonText", "Track"), FOnGetContent::CreateSP(this, &FMotionTimelineTrack_TagsPanel::BuildNotifiesPanelSubMenu, TrackIndex), MakeAttributeSP(SlotBox.Get(), &SWidget::IsHovered))
//				];
//		}
//
//		if (PendingRenameTrackIndex == TrackIndex)
//		{
//			TWeakPtr<SInlineEditableTextBlock> WeakInlineEditableTextBlock = InlineEditableTextBlock;
//			InlineEditableTextBlock->RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &FMotionTimelineTrack_TagsPanel::HandlePendingRenameTimer, WeakInlineEditableTextBlock));
//		}
//
//		TrackIndex++;
//	}
//}
//
//EActiveTimerReturnType FMotionTimelineTrack_TagsPanel::HandlePendingRenameTimer(double InCurrentTime, float InDeltaTime, TWeakPtr<SInlineEditableTextBlock> InInlineEditableTextBlock)
//{
//	if (InInlineEditableTextBlock.IsValid())
//	{
//		InInlineEditableTextBlock.Pin()->EnterEditingMode();
//	}
//
//	PendingRenameTrackIndex = INDEX_NONE;
//
//	return EActiveTimerReturnType::Stop;
//}
//
//TSharedRef<SWidget> FMotionTimelineTrack_TagsPanel::BuildNotifiesPanelSubMenu(int32 InTrackIndex)
//{
//	UAnimSequenceBase* AnimSequence = GetModel()->GetAnimSequenceBase();
//
//	FMenuBuilder MenuBuilder(true, GetModel()->GetCommandList());
//
//	MenuBuilder.BeginSection("NotifyTrack", LOCTEXT("NotifyTrackMenuSection", "Notify Track"));
//	{
//		MenuBuilder.AddMenuEntry(
//			FMotionSequenceTimelineCommands::Get().InsertNotifyTrack->GetLabel(),
//			FMotionSequenceTimelineCommands::Get().InsertNotifyTrack->GetDescription(),
//			FMotionSequenceTimelineCommands::Get().InsertNotifyTrack->GetIcon(),
//			FUIAction(
//				FExecuteAction::CreateSP(this, &FMotionTimelineTrack_TagsPanel::InsertTrack, InTrackIndex)
//			)
//		);
//
//		if (AnimSequence->AnimNotifyTracks.Num() > 1)
//		{
//			MenuBuilder.AddMenuEntry(
//				FMotionSequenceTimelineCommands::Get().RemoveNotifyTrack->GetLabel(),
//				FMotionSequenceTimelineCommands::Get().RemoveNotifyTrack->GetDescription(),
//				FMotionSequenceTimelineCommands::Get().RemoveNotifyTrack->GetIcon(),
//				FUIAction(
//					FExecuteAction::CreateSP(this, &FMotionTimelineTrack_TagsPanel::RemoveTrack, InTrackIndex)
//				)
//			);
//		}
//	}
//	MenuBuilder.EndSection();
//
//	return MenuBuilder.MakeWidget();
//}
//
//void FMotionTimelineTrack_TagsPanel::InsertTrack(int32 InTrackIndexToInsert)
//{
//	UAnimSequenceBase* AnimSequence = GetModel()->GetAnimSequenceBase();
//
//	FScopedTransaction Transaction(LOCTEXT("InsertTagTrack", "Insert Tag Track"));
//	AnimSequence->Modify();
//
//	// before insert, make sure everything behind is fixed
//	for (int32 TrackIndex = InTrackIndexToInsert; TrackIndex < AnimSequence->AnimNotifyTracks.Num(); ++TrackIndex)
//	{
//		FAnimNotifyTrack& Track = AnimSequence->AnimNotifyTracks[TrackIndex];
//
//		const int32 NewTrackIndex = TrackIndex + 1;
//
//		for (FAnimNotifyEvent* Notify : Track.Notifies)
//		{
//			// fix notifies indices
//			Notify->TrackIndex = NewTrackIndex;
//		}
//
//		for (FAnimSyncMarker* SyncMarker : Track.SyncMarkers)
//		{
//			// fix notifies indices
//			SyncMarker->TrackIndex = NewTrackIndex;
//		}
//	}
//
//	FAnimNotifyTrack NewItem;
//	NewItem.TrackName = FMotionTimelineTrack_Tags::GetNewTrackName(AnimSequence);
//	NewItem.TrackColor = FLinearColor::White;
//
//	AnimSequence->AnimNotifyTracks.Insert(NewItem, InTrackIndexToInsert);
//
//	// Request a rename on rebuild
//	PendingRenameTrackIndex = InTrackIndexToInsert;
//
//	Update();
//}
//
//void FMotionTimelineTrack_TagsPanel::RemoveTrack(int32 InTrackIndexToRemove)
//{
//	UAnimSequenceBase* AnimSequence = GetModel()->GetAnimSequenceBase();
//
//	if (AnimSequence->AnimNotifyTracks.IsValidIndex(InTrackIndexToRemove))
//	{
//		if (AnimSequence->AnimNotifyTracks[InTrackIndexToRemove].Notifies.Num() == 0)
//		{
//			FScopedTransaction Transaction(LOCTEXT("RemoveNotifyTrack", "Remove Notify Track"));
//			AnimSequence->Modify();
//
//			// before insert, make sure everything behind is fixed
//			for (int32 TrackIndex = InTrackIndexToRemove + 1; TrackIndex < AnimSequence->AnimNotifyTracks.Num(); ++TrackIndex)
//			{
//				FAnimNotifyTrack& Track = AnimSequence->AnimNotifyTracks[TrackIndex];
//				const int32 NewTrackIndex = TrackIndex - 1;
//
//				for (FAnimNotifyEvent* Notify : Track.Notifies)
//				{
//					// fix notifies indices
//					Notify->TrackIndex = NewTrackIndex;
//				}
//
//				for (FAnimSyncMarker* SyncMarker : Track.SyncMarkers)
//				{
//					// fix notifies indices
//					SyncMarker->TrackIndex = NewTrackIndex;
//				}
//			}
//
//			AnimSequence->AnimNotifyTracks.RemoveAt(InTrackIndexToRemove);
//
//			Update();
//		}
//	}
//}
//
//void FMotionTimelineTrack_TagsPanel::Update()
//{
//	SetHeight((float)GetModel()->GetAnimSequenceBase()->AnimNotifyTracks.Num() * NotificationTrackHeight);
//	RefreshOutlinerWidget();
//	if (MotionTagPanel.IsValid())
//	{
//		MotionTagPanel->Update();
//	}
//}
//
//void FMotionTimelineTrack_TagsPanel::HandleNotifyChanged()
//{
//	SetHeight((float)GetModel()->GetAnimSequenceBase()->AnimNotifyTracks.Num() * NotificationTrackHeight);
//	RefreshOutlinerWidget();
//}
//
//void FMotionTimelineTrack_TagsPanel::OnCommitTrackName(const FText& InText, ETextCommit::Type CommitInfo, int32 TrackIndexToName)
//{
//	UAnimSequenceBase* AnimSequence = GetModel()->GetAnimSequenceBase();
//	if (AnimSequence->AnimNotifyTracks.IsValidIndex(TrackIndexToName))
//	{
//		FScopedTransaction Transaction(FText::Format(LOCTEXT("RenameTagTrack", "Rename Tag Track to '{0}'"), InText));
//		AnimSequence->Modify();
//
//		FText TrimText = FText::TrimPrecedingAndTrailing(InText);
//		AnimSequence->AnimNotifyTracks[TrackIndexToName].TrackName = FName(*TrimText.ToString());
//	}
//}
//
//EVisibility FMotionTimelineTrack_TagsPanel::OnGetTimingNodeVisibility(ETimingElementType::Type ElementType) const
//{
//	return StaticCastSharedRef<FMotionModel_AnimSequenceBase>(GetModel())->IsNotifiesTimingElementDisplayEnabled(ElementType) ? EVisibility::Visible : EVisibility::Hidden;
//}
//
//TSharedRef<SMotionTagPanel> FMotionTimelineTrack_TagsPanel::GetAnimNotifyPanel()
//{
//	if (!MotionTagPanel.IsValid())
//	{
//		UAnimMontage* AnimMontage = Cast<UAnimMontage>(GetModel()->GetAnimSequenceBase());
//		bool bChildAnimMontage = AnimMontage && AnimMontage->HasParentAsset();
//
//		MotionTagPanel = SNew(SMotionTagPanel, GetModel())
//			.IsEnabled(!bChildAnimMontage)
//			.Sequence(GetModel()->GetAnimSequenceBase())
//			.InputMin(this, &FMotionTimelineTrack_TagsPanel::GetMinInput)
//			.InputMax(this, &FMotionTimelineTrack_TagsPanel::GetMaxInput)
//			.ViewInputMin(this, &FMotionTimelineTrack_TagsPanel::GetViewMinInput)
//			.ViewInputMax(this, &FMotionTimelineTrack_TagsPanel::GetViewMaxInput)
//			.OnGetScrubValue(this, &FMotionTimelineTrack_TagsPanel::GetScrubValue)
//			.OnSelectionChanged(this, &FMotionTimelineTrack_TagsPanel::SelectObjects)
//			.OnSetInputViewRange(this, &FMotionTimelineTrack_TagsPanel::OnSetInputViewRange)
//			//.OnInvokeTab(GetModel()->OnInvokeTab)
//			.OnSnapPosition(&GetModel().Get(), &FMotionModel::Snap)
//			//.OnGetTimingNodeVisibility(this, &FMotionTimelineTrack_TagsPanel::OnGetTimingNodeVisibility)
//			.OnTagsChanged_Lambda([this]()
//				{
//					Update();
//					GetModel()->OnTracksChanged().Broadcast();
//				});
//
//		GetModel()->GetAnimSequenceBase()->RegisterOnNotifyChanged(UAnimSequenceBase::FOnNotifyChanged::CreateSP(this, &FMotionTimelineTrack_TagsPanel::HandleNotifyChanged));
//	}
//
//	return MotionTagPanel.ToSharedRef();
//}
//
//#undef LOCTEXT_NAMESPACE