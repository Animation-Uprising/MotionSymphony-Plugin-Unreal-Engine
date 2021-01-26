#pragma once

#include "Templates/SharedPointer.h"
#include "ITimeSlider.h"
#include "PersonaDelegates.h"
#include "UObject/GCObject.h"
#include "Containers/ArrayView.h"


class FAnimTimelineTrack;
enum class EViewRangeInterpolation;
class UAnimSequenceBase;
class FPreviewScene;
class UDebugSkelMeshComponent;
class FUICommandList;
class UEditorAnimBaseObj;

class FMotionModel : public TSharedFromThis<FMotionModel>, public FGCObject 
{
public:
	FMotionModel(const FPreviewScene* InPreviewScene, const UDebugSkelMeshComponent* InPreviewSkeleton, 
		const TSharedRef<FUICommandList>& InCommandList);
};