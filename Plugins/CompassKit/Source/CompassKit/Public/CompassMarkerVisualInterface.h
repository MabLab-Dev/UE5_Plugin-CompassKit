// Copyright MabLab. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CompassMarkerTypes.h"
#include "CompassMarkerVisualInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCompassMarkerVisualInterface : public UInterface
{
    GENERATED_BODY()
};

class COMPASSKIT_API ICompassMarkerVisualInterface
{
    GENERATED_BODY()

public:
    // Called once when marker is created
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CompassKit|Marker")
    void InitializeFromMarkerData(const FCompassMarkerData& MarkerData);

    // Called on tick or when distance changes
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CompassKit|Marker")
    void UpdateDistanceDisplay(float Distance);
};