// Copyright MabLab 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompassKitStatics.generated.h"

class UCompassLocalPlayerSubsystem;
class APlayerController;

/**
 * Blueprint function library providing utility/helper functions for the CompassKit
 */
UCLASS()
class COMPASSKIT_API UCompassKitStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
    /**
     * Finds the Compass Subsystem associated with the specified controller if they have one.
     *
     * @param the Player Controller whose Compass Subsystem you want to access
     * @return The found Compass Subsystem, or nullptr if none was found
     */
    UFUNCTION(BlueprintPure, Category = "CompassKit")
    static UCompassLocalPlayerSubsystem* GetCompassSubsystemForPlayer(const APlayerController* PlayerController);
	
};
