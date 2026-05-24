// Copyright MabLab. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "CompassMarkerTypes.generated.h"

UENUM(BlueprintType)
enum class ECompassProjectionType : uint8
{
    Perspective_3D,     //Uses vector dot projection on compass lateral
    Flat_2D,            //Uses raw angular delta
};

UENUM(BlueprintType)
enum class ECompassMarkerType : uint8
{
    Cardinal,
    Point,
//    Circle,
//    Box,
};


USTRUCT(BlueprintType)
struct FCompassVisibilityPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit")
    TArray<FName> VisibilityChannels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit")
    bool bClampOutsideView = false;

    // Distance filtering
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit")
    float MinimumDistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit")
    float MaximumDistance = 0.f;
};

USTRUCT(BlueprintType)
struct FCompassMarkerData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, meta=(NeverAsPin), Category = "CompassKit")
    FGuid MarkerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (ToolTip = "Marker icon as text, defaults to this if image not specified"))
    FName TextAsIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (ToolTip = "Marker icon as image, defaults to text if not specified"))
    UTexture2D* ImageAsIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (ToolTip = "A text label for the marker"))
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (ToolTip = "Specifiy a marker type, point marker, cardinal direction or area markers"))
    ECompassMarkerType MarkerType = ECompassMarkerType::Cardinal;

    // Cardinal
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (Units = "Degrees", UIMin = "0.0", UIMax = "360.0", EditCondition = "MarkerType==ECompassMarkerType::Cardinal", EditConditionHides, ToolTip = "Example values: 0=North ; 90=East ; 180=South ; 270=West"))
    float Direction;

    // Point
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Point", EditConditionHides, ToolTip = "A hardcoded world location, choosing target actor overrides this"))
    FVector WorldLocation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Point", EditConditionHides, ToolTip = "Actor reference for markers that can move"))
    TWeakObjectPtr<AActor> TargetActor;

/*    // Circle
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Circle", EditConditionHides))
    FVector AreaCenter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Circle", EditConditionHides))
    float AreaRadius;

    // Box
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Box", EditConditionHides))
    FVector BoxCenter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (EditCondition = "MarkerType==ECompassMarkerType::Box", EditConditionHides))
    FVector BoxExtents;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit")
    FCompassVisibilityPolicy Visibility;
*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CompassKit", meta = (ToolTip = "FName tag for custom categorization"))
    FName CategoryTag;
};

USTRUCT(BlueprintType)
struct FCompassRuntimeCache
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "CompassKit")
    float Distance = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "CompassKit")
    float SignedAngle = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "CompassKit")
    FVector2D DirectionalOffset = FVector2D::ZeroVector;

    FCompassRuntimeCache() = default;

    FCompassRuntimeCache(
        float InDistance,
        float InSignedAngle,
        const FVector2D& InDirectionalOffset
    )
        : Distance(InDistance)
        , SignedAngle(InSignedAngle)
        , DirectionalOffset(InDirectionalOffset)
    {
    }
};

struct FRelativeOffsetResult
{
    float HorizontalOffset = 0.f;
    float VerticalOffset = 0.f;
    float Distance = 0.f;
    float FrontBackFactor = 0.f;
    float SideFactor = 0.f;
    float SignedAngleDegrees = 0.f;
};