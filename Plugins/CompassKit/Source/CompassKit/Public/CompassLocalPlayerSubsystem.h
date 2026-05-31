// Copyright MabLab 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "CompassMarkerTypes.h"
#include "CompassLocalPlayerSubsystem.generated.h"

class UUserWidget; // Navigation widgets (CompassBar, ScreenView, Minimap)

/**
 * UCompassLocalPlayerSubsystem
 * Canonical manager for compass markers and runtime math.
 */

UCLASS()
class COMPASSKIT_API UCompassLocalPlayerSubsystem : public ULocalPlayerSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
    /* -------------------- Delegates -------------------- */

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerAdded, const FGuid&, MarkerId);
    UPROPERTY(BlueprintAssignable, Category = "CompassKit|Events")
    FOnMarkerAdded OnMarkerAdded;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerRemoved, const FGuid&, MarkerId);
    UPROPERTY(BlueprintAssignable, Category = "CompassKit|Events")
    FOnMarkerRemoved OnMarkerRemoved;

    // Broadcast only when marker state changes (not every frame)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMarkerUpdated, const FGuid&, MarkerId);
    UPROPERTY(BlueprintAssignable, Category = "CompassKit|Events")
    FOnMarkerUpdated OnMarkerUpdated;

    /* -------------------- Marker Lifecycle -------------------- */

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Marker Lifecycle")
    FGuid AddMarker(const FCompassMarkerData& MarkerData);

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Marker Lifecycle")
    FGuid UpdateMarker(const FCompassMarkerData& MarkerData);

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Marker Lifecycle")
    void RemoveMarker(const FGuid& MarkerId);

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Marker Lifecycle")
    void ClearMarkers();

    /* -------------------- Data Queries -------------------- */

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Data")
    bool GetMarkerData(const FGuid& MarkerId, FCompassMarkerData& MarkerData) const;

    /* -------------------- Runtime Queries -------------------- */

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Runtime")
    bool GetMarkerRuntimeCache(const FGuid& MarkerId, FCompassRuntimeCache& OutCache) const;

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Runtime")
    float GetMarkerDistance(const FGuid& MarkerId) const;

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Runtime")
    float GetMarkerProjectedOffset(const FGuid& MarkerId, ECompassProjectionType CompassProjectionType, float ViewHalfAngle) const;

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Runtime")
    bool IsMarkerWithinView(const FGuid& MarkerId, float ViewHalfAngle) const;

    /* -------------------- Widget Subscription -------------------- */

    // Register a navigation widget (CompassBar, ScreenView, Minimap)
    UFUNCTION(BlueprintCallable, Category = "CompassKit|Subscription")
    void SubscribeNavigationWidget(UUserWidget* NavigationWidget);

    UFUNCTION(BlueprintCallable, Category = "CompassKit|Subscription")
    void UnsubscribeNavigationWidget(UUserWidget* NavigationWidget);

    /* -------------------- Math -------------------- */

    static FRelativeOffsetResult CompareRelativeOffsets(const FTransform& TransformA, const FTransform& TransformB, bool bTransformBAtInfinity);

protected:
    /* -------------------- Internal State -------------------- */

    UPROPERTY()
    TArray<TWeakObjectPtr<UUserWidget>> SubscribedNavigationWidgets;

    UPROPERTY()
    TMap<FGuid, FCompassMarkerData> MarkerRegistry;

    TMap<FGuid, FCompassRuntimeCache> RuntimeCache;

    /* -------------------- Tick -------------------- */

    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual TStatId GetStatId() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;	

private:
    /* -------------------- Subscription Helpers -------------------- */
    bool HasActiveSubscribers() const;
    void PruneInvalidSubscribers();

    /* -------------------- Lifecycle Helpers -------------------- */
    void HandleInvalidActors();
    void RebroadcastExistingMarkers();

    /* -------------------- Helper Resolvers -------------------- */
    FTransform GetPlayerCameraTransform() const;
    FTransform GetPlayerControllerTransform() const;
    FVector    GetPlayerPawnLocation() const;
    FTransform ResolveMarkerTransform(const FCompassMarkerData& MarkerData) const;
};
