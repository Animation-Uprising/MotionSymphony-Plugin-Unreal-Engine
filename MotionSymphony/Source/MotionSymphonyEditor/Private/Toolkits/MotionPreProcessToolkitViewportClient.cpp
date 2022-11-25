// Copyright 2020-2021 Kenneth Claassen. All Rights Reserved.

#include "MotionPreProcessToolkitViewportClient.h"
#include "Engine.h"
#include "EngineGlobals.h"
#include "Engine/EngineTypes.h"
#include "Engine/CollisionProfile.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "AssetEditorModeManager.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "Editor/AdvancedPreviewScene/Public/AssetViewerSettings.h"
#include "Utils.h"
#include "DrawDebugHelpers.h"
#include "MotionMatchingUtil/MMBlueprintFunctionLibrary.h"


#define LOCTEXT_NAMESPACE "MotionPreProcessToolkit"

FMotionPreProcessToolkitViewportClientRoot::FMotionPreProcessToolkitViewportClientRoot(const TWeakPtr<class SEditorViewport>& InEditorViewportWidget)
	: FEditorViewportClient(nullptr)
	//: FEditorViewportClient(new FAssetEditorModeManager(), nullptr, InEditorViewportWidget)
{
	SetViewModes(VMI_Lit, VMI_Lit);
	SetViewportType(LVT_Perspective);
	SetInitialViewTransform(LVT_Perspective, FVector(-300.0f, 300.0f, 150.0f), FRotator(0.0f, -45.0f, 0.0f), 0.0f);
}

FMotionPreProcessToolkitViewportClientRoot::~FMotionPreProcessToolkitViewportClientRoot() {}


FBox FMotionPreProcessToolkitViewportClient::GetDesiredFocusBounds() const
{
	return AnimatedRenderComponent->Bounds.GetBox();
}

FMotionPreProcessToolkitViewportClient::FMotionPreProcessToolkitViewportClient(const TAttribute<UMotionDataAsset*>& InMotionDataAsset,
	TWeakPtr<FMotionPreProcessToolkit> InMotionPreProcessToolkitPtr)
	: MotionPreProcessToolkitPtr(InMotionPreProcessToolkitPtr), CurrentMotionConfig(nullptr), bShowPivot(false), bShowMatchBones(true),
	bShowTrajectory(true), bShowPose(false), bShowOptimizationDebug(false)
{
	MotionData = InMotionDataAsset;

	if(MotionData.Get())
	{
		CurrentMotionConfig = MotionData.Get()->MotionMatchConfig;
	}

	PreviewScene = &OwnedPreviewScene;

	SetRealtime(true);
	SetupAnimatedRenderComponent();

	//Get the default asset viewer settings from the editor and check that they are valid
	DefaultSettings = UAssetViewerSettings::Get();
	const int32 CurrentProfileIndex = 0;
	ensureMsgf(DefaultSettings->Profiles.IsValidIndex(CurrentProfileIndex), TEXT("Invalid default settings pointer or current profile index"));
	FPreviewSceneProfile& Profile = DefaultSettings->Profiles[CurrentProfileIndex];

	SetupSkylight(Profile);
	SetupSkySphere(Profile);
	SetupPostProcess(Profile);
	SetupFloor();

	PreviewScene->SetLightDirection(Profile.DirectionalLightRotation);

	DrawHelper.bDrawGrid = true;
	EngineShowFlags.DisableAdvancedFeatures();
	EngineShowFlags.SetCompositeEditorPrimitives(true);
}

void FMotionPreProcessToolkitViewportClient::Draw(const FSceneView* SceneView, FPrimitiveDrawInterface* DrawInterface)
{
	FEditorViewportClient::Draw(SceneView, DrawInterface);

	if (bShowPivot)
	{
		FUnrealEdUtils::DrawWidget(SceneView, DrawInterface,
			AnimatedRenderComponent->GetComponentTransform().ToMatrixWithScale(),
			0, 0, EAxisList::Screen, EWidgetMovementMode::WMM_Translate);
	}

	UWorld* World = PreviewScene->GetWorld();
	FlushPersistentDebugLines(World);

	if (bShowMatchBones)
	{
		DrawMatchBones(DrawInterface, World);
	}

	if(bShowTrajectory)
	{
		DrawCurrentTrajectory(DrawInterface);
	}

	if (bShowPose)
	{
		DrawCurrentPose(DrawInterface, World);
	}

	if(bShowOptimizationDebug)
	{
		DrawOptimisationDebug(DrawInterface, World);
	}
}

void FMotionPreProcessToolkitViewportClient::DrawCanvas(FViewport& InViewport, FSceneView& SceneView, FCanvas& Canvas)
{
	FEditorViewportClient::DrawCanvas(InViewport, SceneView, Canvas);

	const bool bIsHitTesting = Canvas.IsHitTesting();
	if (!bIsHitTesting)
	{
		Canvas.SetHitProxy(nullptr);
	}
	
	float YPos = 42.0f;

	UMotionDataAsset* ActiveMotionData = MotionPreProcessToolkitPtr.Pin()->GetActiveMotionDataAsset();

	if (!ActiveMotionData 
	 || !ActiveMotionData->bIsProcessed)
	{
		return;
	}

	const int PreviewIndex = MotionPreProcessToolkitPtr.Pin()->PreviewPoseCurrentIndex;

	if (PreviewIndex < 0 
	 || PreviewIndex > ActiveMotionData->Poses.Num())
	{
		return;
	}

	FPoseMotionData& Pose = ActiveMotionData->Poses[PreviewIndex];

	FText PoseText = FText::Format(LOCTEXT("PoseText", "Anim Name: {5} \nPose Id: {0} \nAnim Id: {1}  \nLast Pose Id: {2} \nNext Pose Id: {3} \nCandidate Set Id: {6} \nCost Multiplier: {4}"), 
		Pose.PoseId, Pose.AnimId, Pose.LastPoseId, Pose.NextPoseId, Pose.Favour, MotionPreProcessToolkitPtr.Pin()->CurrentAnimName, Pose.CandidateSetId);

	FCanvasTextItem PoseTextItem(FVector2D(6.0f, YPos), PoseText, GEngine->GetSmallFont(), FLinearColor::White);
	PoseTextItem.EnableShadow(FLinearColor::Black);
	PoseTextItem.Draw(&Canvas);
	YPos += 18.0f * 6.0f;
	
	if(Pose.bDoNotUse)
	{
		static const FText PoseDoNotUseHelpStr = LOCTEXT("PoseDoNotUseHelp", "DoNotUse\n");

		FCanvasTextItem TextItem(FVector2D(6.0f, YPos), PoseDoNotUseHelpStr, GEngine->GetSmallFont(), FLinearColor::Red);
		TextItem.EnableShadow(FLinearColor::Black);
		TextItem.Draw(&Canvas);
		YPos += 36.0f;
	}
}

void FMotionPreProcessToolkitViewportClient::Tick(float DeltaSeconds)
{
	if (AnimatedRenderComponent.IsValid())
	{
		AnimatedRenderComponent->UpdateBounds();

		FTransform ComponentTransform = FTransform::Identity;

		if (FMotionAnimAsset* CurrentMotionAnim = MotionPreProcessToolkitPtr.Pin().Get()->GetCurrentMotionAnim())
		{
			CurrentMotionAnim->GetRootBoneTransform(ComponentTransform, AnimatedRenderComponent->GetPosition());
		}

		if (DeltaSeconds > 0.000001f)
		{
			MotionPreProcessToolkitPtr.Pin()->FindCurrentPose(AnimatedRenderComponent->GetPosition());
		}

		if(CurrentMotionConfig)
		{
			FTransform CorrectionTransform(FTransform::Identity);

			//Correction for non-standard model facing
			UMMBlueprintFunctionLibrary::TransformFromUpForwardAxis(CorrectionTransform, CurrentMotionConfig->UpAxis, CurrentMotionConfig->ForwardAxis);

			//Correction for root rotation
			if(MotionPreProcessToolkitPtr.Pin()->CurrentAnimIndex > -1)
			{
				FTransform RootTransform = AnimatedRenderComponent->GetReferenceSkeleton().GetRefBonePose()[0];
				CorrectionTransform *= RootTransform.Inverse();
			}
	
			ComponentTransform.ConcatenateRotation(CorrectionTransform.GetRotation());
		}
		
		AnimatedRenderComponent->SetWorldTransform(ComponentTransform, false);
	}

	FMotionPreProcessToolkitViewportClientRoot::Tick(DeltaSeconds);

	OwnedPreviewScene.GetWorld()->Tick(LEVELTICK_All, DeltaSeconds);
}

bool FMotionPreProcessToolkitViewportClient::InputKey(FViewport* InViewport, int32 ControllerId, FKey Key,
	EInputEvent Event, float AmountDepressed, bool bGamepad)
{
	const bool bHandled = false;

#if ENGINE_MAJOR_VERSION >= 5 & ENGINE_MINOR_VERSION >= 1
	const FInputKeyEventArgs InputArgs(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
	return (bHandled) ? true : FEditorViewportClient::InputKey(InputArgs);
#else
	return (bHandled) ? true : FEditorViewportClient::InputKey(InViewport, ControllerId, Key, Event, AmountDepressed, bGamepad);
#endif
}

FLinearColor FMotionPreProcessToolkitViewportClient::GetBackgroundColor() const
{
	return FColor(55, 55, 55);
}

void FMotionPreProcessToolkitViewportClient::ToggleShowPivot()
{
	bShowPivot = !bShowPivot;
}

bool FMotionPreProcessToolkitViewportClient::IsShowPivotChecked() const
{
	return bShowPivot;
}

void FMotionPreProcessToolkitViewportClient::ToggleShowMatchBones()
{
	bShowMatchBones = !bShowMatchBones;
}

bool FMotionPreProcessToolkitViewportClient::IsShowMatchBonesChecked() const
{
	return bShowMatchBones;
}

void FMotionPreProcessToolkitViewportClient::ToggleShowTrajectory()
{
	bShowTrajectory = !bShowTrajectory;
}

bool FMotionPreProcessToolkitViewportClient::IsShowTrajectoryChecked() const
{
	return bShowTrajectory;
}

void FMotionPreProcessToolkitViewportClient::ToggleShowPose()
{
	bShowPose = !bShowPose;
}

bool FMotionPreProcessToolkitViewportClient::IsShowPoseChecked() const
{
	return bShowPose;
}

void FMotionPreProcessToolkitViewportClient::ToggleShowOptimizationDebug()
{
	bShowOptimizationDebug = !bShowOptimizationDebug;
}

bool FMotionPreProcessToolkitViewportClient::IsShowOptimizationDebugChecked() const
{
	return bShowOptimizationDebug;
}

void FMotionPreProcessToolkitViewportClient::DrawMatchBones(FPrimitiveDrawInterface* DrawInterface, const UWorld* World) const
{
	if (!DrawInterface 
	 || !World 
	 || !MotionPreProcessToolkitPtr.IsValid())
	 {
		return;
	}

	UMotionDataAsset* ActiveMotionData = MotionPreProcessToolkitPtr.Pin()->GetActiveMotionDataAsset();

	if (!ActiveMotionData)
	{
		return;
	}

	UDebugSkelMeshComponent* DebugSkeletalMesh = MotionPreProcessToolkitPtr.Pin()->GetPreviewSkeletonMeshComponent();
	UMotionMatchConfig* MMConfig = ActiveMotionData->MotionMatchConfig;

	if (!MMConfig || !DebugSkeletalMesh)
	{
		return;
	}

	for(const FBoneReference& BoneRef : MMConfig->PoseBones)
	{
		const int32 BoneIndex = DebugSkeletalMesh->GetBoneIndex(BoneRef.BoneName);
		FVector JointPos = DebugSkeletalMesh->GetBoneTransform(BoneIndex).GetLocation();

		DrawDebugSphere(World, JointPos, 8.0f, 8, FColor::Yellow, true, -1, 0);
	}
}

void FMotionPreProcessToolkitViewportClient::DrawCurrentTrajectory(FPrimitiveDrawInterface* DrawInterface) const
{
	if (!MotionPreProcessToolkitPtr.IsValid())
	{
		return;
	}

	MotionPreProcessToolkitPtr.Pin()->DrawCachedTrajectoryPoints(DrawInterface);
}

void FMotionPreProcessToolkitViewportClient::DrawCurrentPose(FPrimitiveDrawInterface* DrawInterface, const UWorld* World) const
{
	if (!DrawInterface 
	 || !World 
	 || !MotionPreProcessToolkitPtr.IsValid())
	{
		return;
	}

	UMotionDataAsset* ActiveMotionData = MotionPreProcessToolkitPtr.Pin()->GetActiveMotionDataAsset();

	if (!ActiveMotionData || !ActiveMotionData->bIsProcessed)
	{
		return;
	}
	
	const int PreviewIndex = MotionPreProcessToolkitPtr.Pin()->PreviewPoseCurrentIndex;

	if (PreviewIndex < 0 || PreviewIndex > ActiveMotionData->Poses.Num())
	{
		return;
	}

	UDebugSkelMeshComponent* DebugSkeletalMesh = MotionPreProcessToolkitPtr.Pin()->GetPreviewSkeletonMeshComponent();
	if (!DebugSkeletalMesh)
	{
		return;
	}

	const FTransform PreviewTransform = DebugSkeletalMesh->GetComponentTransform();
	FPoseMotionData& Pose = ActiveMotionData->Poses[PreviewIndex];

	//Draw all pre-processed pose joint data relative to the character
	for (int i = 0; i < Pose.JointData.Num(); ++i)
	{
		FJointData& JointData = Pose.JointData[i];

		FVector BonePos = PreviewTransform.TransformPosition(JointData.Position);

		DrawDebugSphere(World, BonePos, 8.0f, 8, FColor::Blue, true, -1, 0);

		FVector EndPoint = BonePos + PreviewTransform.TransformVector(JointData.Velocity * 0.3333f);

		DrawDebugDirectionalArrow(World, BonePos, EndPoint, 20.0f, FColor::Blue, true, -1, 0, 1.5f);
	}

	if (Pose.Trajectory.Num() > 0)
	{
		FVector LastPointPos = PreviewTransform.TransformPosition(Pose.Trajectory[0].Position);
		for (FTrajectoryPoint& Point : Pose.Trajectory)
		{
			FVector PointPos = PreviewTransform.TransformPosition(Point.Position);

			DrawDebugSphere(World, PointPos, 5.0f, 8, FColor::Red, true, -1, 0);

			DrawInterface->DrawLine(LastPointPos, PointPos, FLinearColor::Red, 
				ESceneDepthPriorityGroup::SDPG_Foreground, 3.0f);

			FQuat Rotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(Point.RotationZ + 90.0f));
			FVector ArrowVector = PreviewTransform.TransformVector(Rotation * (FVector::ForwardVector * 50.0f));

			DrawDebugDirectionalArrow(World, PointPos, PointPos + ArrowVector, 20.0f, FColor::Red,
				true, -1, 0, 1.5f);

			LastPointPos = PointPos;
		}
	}
}

void FMotionPreProcessToolkitViewportClient::DrawOptimisationDebug(FPrimitiveDrawInterface* DrawInterface, const UWorld* World) const
{
	if (!DrawInterface || !World || !MotionPreProcessToolkitPtr.IsValid())
	{
		return;
	}

	UMotionDataAsset* ActiveMotionData = MotionPreProcessToolkitPtr.Pin()->GetActiveMotionDataAsset();

	if (!ActiveMotionData 
	|| !ActiveMotionData->bIsProcessed
	|| !ActiveMotionData->OptimisationModule 
	|| !ActiveMotionData->bOptimize)
	{
		return;
	}

	ActiveMotionData->OptimisationModule->DrawDebug(DrawInterface, World, ActiveMotionData);
}

void FMotionPreProcessToolkitViewportClient::SetCurrentTrajectory(const FTrajectory InTrajectory)
{
	CurrentTrajectory = InTrajectory;
}

UDebugSkelMeshComponent* FMotionPreProcessToolkitViewportClient::GetPreviewComponent() const
{
	return AnimatedRenderComponent.Get();
}

void FMotionPreProcessToolkitViewportClient::RequestFocusOnSelection(bool bInstant)
{
	FocusViewportOnBox(GetDesiredFocusBounds(), bInstant);
}

void FMotionPreProcessToolkitViewportClient::SetupAnimatedRenderComponent()
{
	if (AnimatedRenderComponent.IsValid())
	{
		return;
	}

	AnimatedRenderComponent = NewObject<UDebugSkelMeshComponent>();
	AnimatedRenderComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	if (!MotionData.Get() || !MotionData.Get()->MotionMatchConfig)
	{
		return;
	}

	USkeleton* Skeleton = MotionData.Get()->MotionMatchConfig->GetSourceSkeleton();

	if (Skeleton)
	{
		USkeletalMesh* SkelMesh = Skeleton->GetPreviewMesh();

		if (SkelMesh)
		{
			AnimatedRenderComponent->SetSkeletalMesh(SkelMesh);
		}
	}

	AnimatedRenderComponent->UpdateBounds();
	OwnedPreviewScene.AddComponent(AnimatedRenderComponent.Get(), FTransform::Identity);
}

void FMotionPreProcessToolkitViewportClient::SetupSkylight(FPreviewSceneProfile& Profile)
{
	const FTransform Transform(FRotator(0.0f), FVector(0.0f), FVector(1.0f));

	//Add and set up skylight
	static const auto CVarSupportStationarySkylight = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SupportStationarySkylight"));
	const bool bUseSkylight = CVarSupportStationarySkylight->GetValueOnAnyThread() != 0;
	if (bUseSkylight)
	{
		//Setup a skylight
		SkyLightComponent = NewObject<USkyLightComponent>();
		SkyLightComponent->Cubemap = Profile.EnvironmentCubeMap.Get();
		SkyLightComponent->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
		SkyLightComponent->Mobility = EComponentMobility::Movable;
		SkyLightComponent->bLowerHemisphereIsBlack = false;
		SkyLightComponent->Intensity = Profile.SkyLightIntensity;
		PreviewScene->AddComponent(SkyLightComponent, Transform);
		SkyLightComponent->UpdateSkyCaptureContents(PreviewScene->GetWorld());
	}
	else
	{
		//Set up a sphere reflection component if a skylight shouldn't be used
		SphereReflectionComponent = NewObject<USphereReflectionCaptureComponent>();
		SphereReflectionComponent->Cubemap = Profile.EnvironmentCubeMap.Get();
		SphereReflectionComponent->ReflectionSourceType = EReflectionSourceType::SpecifiedCubemap;
		SphereReflectionComponent->Brightness = Profile.SkyLightIntensity;
		PreviewScene->AddComponent(SphereReflectionComponent, Transform);
		SphereReflectionComponent->UpdateReflectionCaptureContents(PreviewScene->GetWorld());
	}
}

void FMotionPreProcessToolkitViewportClient::SetupSkySphere(FPreviewSceneProfile& Profile)
{
	//Large scale to prevent sphere from clipping
	const FTransform SphereTransform(FRotator(0.0f), FVector(0.0f), FVector(2000.0f));
	SkyComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());

	//Set up sky sphere showing the same cube map as used by the sky light
	UStaticMesh* SkySphere = LoadObject<UStaticMesh>(NULL, TEXT(
		"/Engine/EditorMeshes/AssetViewer/Sphere_inversenormals.Sphere_inversenormals"), NULL, LOAD_None, NULL);
	check(SkySphere);

	SkyComponent->SetStaticMesh(SkySphere);

	UMaterial* SkyMaterial = LoadObject<UMaterial>(NULL, TEXT(
		"/Engine/EditorMaterials/AssetViewer/M_SkyBox.M_SkyBox"), NULL, LOAD_None, NULL);
	check(SkyMaterial);

	//Create and setup instanced sky material parameters
	InstancedSkyMaterial = NewObject<UMaterialInstanceConstant>(GetTransientPackage());
	InstancedSkyMaterial->Parent = SkyMaterial;

	UTextureCube* DefaultTexture = LoadObject<UTextureCube>(NULL, TEXT(
		"/Engine/MapTemplates/Sky/SunsetAmbientCubemap.SunsetAmbientCubemap"));

	InstancedSkyMaterial->SetTextureParameterValueEditorOnly(FName("SkyBox"),
		(Profile.EnvironmentCubeMap.Get() != nullptr ? Profile.EnvironmentCubeMap.Get() : DefaultTexture));

	InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("CubemapRotation"),
		Profile.LightingRigRotation / 360.0f);

	InstancedSkyMaterial->SetScalarParameterValueEditorOnly(FName("Intensity"), Profile.SkyLightIntensity);
	InstancedSkyMaterial->PostLoad();
	SkyComponent->SetMaterial(0, InstancedSkyMaterial);
	PreviewScene->AddComponent(SkyComponent, SphereTransform);
}

void FMotionPreProcessToolkitViewportClient::SetupPostProcess(FPreviewSceneProfile& Profile)
{
	const FTransform Transform(FRotator(0.0f), FVector(0.0f), FVector(1.0f));

	//Create and add post process component
	PostProcessComponent = NewObject<UPostProcessComponent>();
	PostProcessComponent->Settings = Profile.PostProcessingSettings;
	PostProcessComponent->bUnbound = true;
	PreviewScene->AddComponent(PostProcessComponent, Transform);
}

void FMotionPreProcessToolkitViewportClient::SetupFloor()
{
	//Create and add a floor mesh for the preview scene
	UStaticMesh* FloorMesh = LoadObject<UStaticMesh>(NULL, TEXT(
		"/Engine/EditorMeshes/AssetViewer/Floor_Mesh.Floor_Mesh"), NULL, LOAD_None, NULL);
	check(FloorMesh);
	FloorMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage());
	FloorMeshComponent->SetStaticMesh(FloorMesh);

	const FTransform FloorTransform(FRotator(0.0f), FVector(0.0f, 0.0f, -1.0f),
		FVector(4.0f, 4.0f, 1.0f));
	PreviewScene->AddComponent(FloorMeshComponent, FloorTransform);
}

#undef LOCTEXT_NAMESPACE