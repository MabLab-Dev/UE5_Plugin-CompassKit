// Copyright MabLab. All Rights Reserved.


#include "CompassLocalPlayerSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/UserWidget.h"

FGuid UCompassLocalPlayerSubsystem::AddMarker(const FCompassMarkerData& MarkerData)
{
    FCompassMarkerData Copy = MarkerData;

    if (!Copy.MarkerId.IsValid())
    {
        Copy.MarkerId = FGuid::NewGuid();
    }

    MarkerRegistry.Add(Copy.MarkerId, Copy);

    OnMarkerAdded.Broadcast(Copy.MarkerId);

    return Copy.MarkerId;
}

FGuid UCompassLocalPlayerSubsystem::UpdateMarker(const FCompassMarkerData& MarkerData)
{
    if (MarkerData.MarkerId.IsValid() && MarkerRegistry.Contains(MarkerData.MarkerId))
    {
        MarkerRegistry[MarkerData.MarkerId] = MarkerData;
        OnMarkerUpdated.Broadcast(MarkerData.MarkerId);

        return MarkerData.MarkerId;
    }

    // Treat update of unknown marker as add
    return AddMarker(MarkerData);
}

void UCompassLocalPlayerSubsystem::RemoveMarker(const FGuid& MarkerId)
{
    if (MarkerRegistry.Remove(MarkerId) > 0)
    {
        RuntimeCache.Remove(MarkerId); // Clean up runtime cache too
        OnMarkerRemoved.Broadcast(MarkerId);
    }
}

bool UCompassLocalPlayerSubsystem::GetMarkerData(const FGuid& MarkerId, FCompassMarkerData& MarkerData) const
{
    if (const FCompassMarkerData* Data = MarkerRegistry.Find(MarkerId))
    {
        MarkerData = *Data;
        return true;
    }
    return false;
}

void UCompassLocalPlayerSubsystem::ClearMarkers()
{
    TArray<FGuid> MarkerIds;

    MarkerRegistry.GetKeys(MarkerIds);

    for (const FGuid& MarkerId : MarkerIds)
    {
        RemoveMarker(MarkerId);
    }
}

bool UCompassLocalPlayerSubsystem::GetMarkerRuntimeCache(const FGuid& MarkerId, FCompassRuntimeCache& OutCache) const
{
    const FCompassRuntimeCache* Cache = RuntimeCache.Find(MarkerId);

    if (!Cache)
    {
        return false;
    }

    OutCache = *Cache;
    return true;
}

float UCompassLocalPlayerSubsystem::GetMarkerDistance(const FGuid& MarkerId) const
{
    if (const FCompassRuntimeCache* Cache = RuntimeCache.Find(MarkerId))
    {
        return Cache->Distance;
    }
    return 0.f;
}

float UCompassLocalPlayerSubsystem::GetMarkerProjectedOffset(const FGuid& MarkerId, ECompassProjectionType CompassProjectionType, float ViewHalfAngle) const
{
    const FCompassRuntimeCache* Cache = RuntimeCache.Find(MarkerId);
    if (!Cache)
    {
        return 0.f;
    }

    switch (CompassProjectionType)
    {
        case ECompassProjectionType::Perspective_3D:
        {
            // Dot product projection (3D perspective mode)
            return Cache->DirectionalOffset.X;
        }

        case ECompassProjectionType::Flat_2D:
        {
            // Angular projection
            if (ViewHalfAngle <= UE_KINDA_SMALL_NUMBER)
            {
                return 0.f;
            }

            return Cache->SignedAngle / ViewHalfAngle;
        }

        default:
            return 0.f;
    }
}

bool UCompassLocalPlayerSubsystem::IsMarkerWithinView(const FGuid& MarkerId, float ViewHalfAngle) const
{
    const FCompassRuntimeCache* Cache = RuntimeCache.Find(MarkerId);
    if (!Cache)
    {
        return false;
    }

    if (ViewHalfAngle <= UE_KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const float NormalizedAngle = Cache->SignedAngle / ViewHalfAngle;

    return FMath::Abs(NormalizedAngle) <= 1.f;
}

void UCompassLocalPlayerSubsystem::SubscribeNavigationWidget(UUserWidget* NavigationWidget)
{
    if (!NavigationWidget)
    {
        return;
    }

    SubscribedNavigationWidgets.AddUnique(NavigationWidget);

    // Rebroadcast existing markers so newly subscribed widgets receive them
    RebroadcastExistingMarkers();
}

void UCompassLocalPlayerSubsystem::UnsubscribeNavigationWidget(UUserWidget* NavigationWidget)
{
    if (NavigationWidget)
    {
        SubscribedNavigationWidgets.Remove(NavigationWidget);
    }
}

FRelativeOffsetResult UCompassLocalPlayerSubsystem::CompareRelativeOffsets(const FTransform& TransformA, const FTransform& TransformB, bool bTransformBAtInfinity)
{
    FRelativeOffsetResult Result;

    const FVector ForwardA = TransformA.GetRotation().GetForwardVector();
    const FVector RightA = TransformA.GetRotation().GetRightVector();
    const FVector UpA = TransformA.GetRotation().GetUpVector();

    FVector DirectionToB;
    float Distance = 0.f;

    if (bTransformBAtInfinity)
    {
        // Cardinal-style marker (direction only)
        DirectionToB = TransformB.GetRotation().GetForwardVector();
        Distance = 0.f;
    }
    else
    {
        // World-space marker
        const FVector ABVector = TransformB.GetLocation() - TransformA.GetLocation();
        Distance = ABVector.Size();
        DirectionToB = ABVector.GetSafeNormal();
    }

    // ----------------------------
    // Signed angle (yaw)
    // ----------------------------
    const float ForwardDot = FVector::DotProduct(ForwardA, DirectionToB);
    const FVector Cross = FVector::CrossProduct(ForwardA, DirectionToB);

    const float AngleRadians = FMath::Atan2(
        FVector::DotProduct(Cross, UpA),
        ForwardDot
    );

    Result.SignedAngleDegrees = FMath::RadiansToDegrees(AngleRadians);

    // ----------------------------
    // Horizontal offset
    // ----------------------------
    Result.HorizontalOffset = FVector::DotProduct(RightA, DirectionToB);

    // ----------------------------
    // Vertical offset (for screen markers later)
    // ----------------------------
    Result.VerticalOffset = FVector::DotProduct(UpA, DirectionToB);

    // ----------------------------
    // Distance
    // ----------------------------
    Result.Distance = Distance;

    return Result;
}


void UCompassLocalPlayerSubsystem::Tick(float DeltaTime)
{
    // Step 1: Prune invalid subscribers
    PruneInvalidSubscribers();

    // Step 2: Remove markers whose actors were destroyed
    HandleInvalidActors();

    // Step 3: Early exit if no active subscribers
    if (!HasActiveSubscribers())
    {
        return;
    }

    // Step 4: Prepare player reference transforms
    const FTransform CameraTransform = GetPlayerCameraTransform();
    const FTransform ControllerTransform = GetPlayerControllerTransform();
    const FVector    PawnLocation = GetPlayerPawnLocation();

    // Step 5: Iterate over all markers in the registry
    for (auto& Pair : MarkerRegistry)
    {
        const FGuid MarkerId = Pair.Key;
        const FCompassMarkerData& MarkerData = Pair.Value;

        // Resolve marker transform depending on type
        const FTransform MarkerTransform = ResolveMarkerTransform(MarkerData);

        const bool bIsCardinal = (MarkerData.MarkerType == ECompassMarkerType::Cardinal);

        const FTransform& ReferenceTransform = bIsCardinal ? ControllerTransform : CameraTransform;

        //Cardinal markers only care about rotation while world markers also care about position
        FRelativeOffsetResult OffsetData = CompareRelativeOffsets(ReferenceTransform, MarkerTransform, bIsCardinal);

        float Distance = 0.f;
        if (MarkerData.MarkerType == ECompassMarkerType::Point)
        {
            //This distance value is different from the one in CompareRelativeOffsets() as this has pawn location as reference rather than camera location
            if (MarkerData.TargetActor.IsValid())
            {
                Distance = FVector::Dist(PawnLocation, MarkerData.TargetActor->GetActorLocation());
            }
            else
            {
                Distance = FVector::Dist(PawnLocation, MarkerTransform.GetLocation());
            }
        }

        FVector2D DirectionalOffset(OffsetData.HorizontalOffset, OffsetData.VerticalOffset);

        float SignedAngle = OffsetData.SignedAngleDegrees;

        // Step 6: Populate runtime cache
        RuntimeCache.Add(MarkerId, FCompassRuntimeCache(Distance, SignedAngle, DirectionalOffset));
    }
}

bool UCompassLocalPlayerSubsystem::IsTickable() const
{
    // Gate ticking based on active subscribers
    return HasActiveSubscribers();
}

TStatId UCompassLocalPlayerSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCompassLocalPlayerSubsystem, STATGROUP_Tickables);
}

UWorld* UCompassLocalPlayerSubsystem::GetTickableGameObjectWorld() const
{
    return GetWorld();
}

bool UCompassLocalPlayerSubsystem::HasActiveSubscribers() const
{
    for (const TWeakObjectPtr<UUserWidget>& WidgetPtr : SubscribedNavigationWidgets)
    {
        if (WidgetPtr.IsValid())
        {
            return true;
        }
    }
    return false;
}

void UCompassLocalPlayerSubsystem::PruneInvalidSubscribers()
{
    SubscribedNavigationWidgets.RemoveAll(
        [](const TWeakObjectPtr<UUserWidget>& WidgetPtr)
        {
            return !WidgetPtr.IsValid();
        });
}

void UCompassLocalPlayerSubsystem::HandleInvalidActors()
{
    TArray<FGuid> MarkersToRemove;

    for (const auto& Pair : MarkerRegistry)
    {
        const FGuid MarkerId = Pair.Key;
        const FCompassMarkerData& MarkerData = Pair.Value;

        if (MarkerData.MarkerType == ECompassMarkerType::Point && !MarkerData.TargetActor.IsValid() && MarkerData.TargetActor.IsStale())
        {
            MarkersToRemove.Add(MarkerId);
        }
    }

    for (const FGuid& MarkerId : MarkersToRemove)
    {
        RemoveMarker(MarkerId);
    }
}

void UCompassLocalPlayerSubsystem::RebroadcastExistingMarkers()
{
    for (const auto& Pair : MarkerRegistry)
    {
        const FGuid MarkerId = Pair.Key;

        OnMarkerAdded.Broadcast(MarkerId);
    }
}

FTransform UCompassLocalPlayerSubsystem::GetPlayerCameraTransform() const
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
        {
            if (PC->PlayerCameraManager)
            {
                return PC->PlayerCameraManager->GetTransform();
            }
        }
    }
    return FTransform::Identity;
}

FTransform UCompassLocalPlayerSubsystem::GetPlayerControllerTransform() const
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
        {
            // Build a transform from control rotation only, ignore location
            const FRotator ControlRot = PC->GetControlRotation();
            return FTransform(ControlRot, FVector::ZeroVector);
        }
    }
    return FTransform::Identity;
}

FVector UCompassLocalPlayerSubsystem::GetPlayerPawnLocation() const
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (APlayerController* PC = LocalPlayer->GetPlayerController(GetWorld()))
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                return Pawn->GetActorLocation();
            }
        }
    }
    return FVector::ZeroVector;
}

FTransform UCompassLocalPlayerSubsystem::ResolveMarkerTransform(const FCompassMarkerData& MarkerData) const
{
    switch (MarkerData.MarkerType)
    {
    case ECompassMarkerType::Cardinal:
    {
        FRotator Rot(0.f, MarkerData.Direction, 0.f);
        return FTransform(Rot, FVector::ZeroVector);
    }

    case ECompassMarkerType::Point:
    {
        // Point markers: prefer actor reference if valid
        if (MarkerData.TargetActor.IsValid())
        {
            return MarkerData.TargetActor->GetActorTransform();
        }
        // Fallback to static world location
        return FTransform(FRotator::ZeroRotator, MarkerData.WorldLocation);
    }
/*
    case ECompassMarkerType::Circle:
    {
        return FTransform(FRotator::ZeroRotator, MarkerData.AreaCenter);
    }

    case ECompassMarkerType::Box:
    {
        return FTransform(FRotator::ZeroRotator, MarkerData.BoxCenter);
    }
*/
    default:
        return FTransform::Identity;
    }
}