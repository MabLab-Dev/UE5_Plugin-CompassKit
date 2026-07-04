// Copyright MabLab 2026. All Rights Reserved.

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

void UCompassLocalPlayerSubsystem::SetProjectionReferenceActor(AActor* Actor)
{
    ProjectionReferenceActor = Actor;
}

void UCompassLocalPlayerSubsystem::ClearProjectionReferenceActor()
{
    ProjectionReferenceActor.Reset();
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

FRelativeNavigationResult UCompassLocalPlayerSubsystem::CalculateRelativeNavigation(const FTransform& ReferenceTransform, const FCompassMarkerData& MarkerData)
{
    FRelativeNavigationResult Result;

    const FVector Forward = ReferenceTransform.GetRotation().GetForwardVector();
    const FVector Right = ReferenceTransform.GetRotation().GetRightVector();
    const FVector Up = ReferenceTransform.GetRotation().GetUpVector();

    FVector DirectionToTarget = FVector::ForwardVector;
    float Distance = 0.f;

    switch (MarkerData.MarkerType)
    {
    case ECompassMarkerType::Cardinal:
    {
        DirectionToTarget = FRotator(0.f, MarkerData.Direction, 0.f).Vector();

        break;
    }
    case ECompassMarkerType::Point:
    {
        FVector TargetLocation = MarkerData.TargetActor.IsValid() ? MarkerData.TargetActor->GetActorLocation() : MarkerData.WorldLocation;

        const FVector ToTarget = (TargetLocation - ReferenceTransform.GetLocation());
        DirectionToTarget = ToTarget.GetSafeNormal();
        Distance = ToTarget.Size();

        break;
    }
    /*
    case ECompassMarkerType::Circle:
    {

    }

    case ECompassMarkerType::Box:
    {

    }
    */
    default:
        break;
    }

    // ----------------------------
    // Signed angle (yaw)
    // ----------------------------

    // Flatten vectors onto the horizontal plane so the signed angle is unaffected by camera pitch.
    const FVector FlatForward = FVector::VectorPlaneProject(Forward, FVector::UpVector).GetSafeNormal();
    const FVector FlatDirection = FVector::VectorPlaneProject(DirectionToTarget, FVector::UpVector).GetSafeNormal();

    const float ForwardDot = FVector::DotProduct(FlatForward, FlatDirection);
    const FVector Cross = FVector::CrossProduct(FlatForward, FlatDirection);

    const float AngleRadians = FMath::Atan2(
        Cross.Z,
        ForwardDot
    );

    Result.SignedAngleDegrees = FMath::RadiansToDegrees(AngleRadians);

    // ----------------------------
    // Horizontal offset
    // ----------------------------
    Result.HorizontalOffset = FVector::DotProduct(Right, DirectionToTarget);

    // ----------------------------
    // Vertical offset (for screen markers later)
    // ----------------------------
    Result.VerticalOffset = FVector::DotProduct(Up, DirectionToTarget);

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

    // Step 4: Prepare player reference data
    const FTransform ProjectionReferenceTransform = ResolveProjectionReferenceTransform();
    const FVector PawnLocation = GetPlayerPawnLocation();

    // Step 5: Iterate over all markers in the registry
    for (auto& Pair : MarkerRegistry)
    {
        const FGuid MarkerId = Pair.Key;
        const FCompassMarkerData& MarkerData = Pair.Value;

        // Calculate relative navigation data for this marker
        const FRelativeNavigationResult NavigationResult = CalculateRelativeNavigation(ProjectionReferenceTransform, MarkerData);

        float PawnDistance = 0.f;
        if (MarkerData.MarkerType == ECompassMarkerType::Point)
        {
            //Distance of the pawn may be different from what you get from CalculateRelativeNavigation(), as that is taken from the camera by default
            if (MarkerData.TargetActor.IsValid())
            {
                PawnDistance = FVector::Dist(PawnLocation, MarkerData.TargetActor->GetActorLocation());
            }
            else
            {
                PawnDistance = FVector::Dist(PawnLocation, MarkerData.WorldLocation);
            }
        }

        FVector2D DirectionalOffset(NavigationResult.HorizontalOffset, NavigationResult.VerticalOffset);

        float SignedAngle = NavigationResult.SignedAngleDegrees;

        // Step 6: Populate runtime cache
        RuntimeCache.Add(MarkerId, FCompassRuntimeCache(PawnDistance, SignedAngle, DirectionalOffset));
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

FTransform UCompassLocalPlayerSubsystem::ResolveProjectionReferenceTransform() const
{
    if (ProjectionReferenceActor.IsValid())
    {
        return ProjectionReferenceActor->GetActorTransform();
    }

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