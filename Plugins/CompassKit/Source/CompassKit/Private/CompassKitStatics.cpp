// Copyright MabLab. All Rights Reserved.

#include "CompassKitStatics.h"
#include "CompassLocalPlayerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

UCompassLocalPlayerSubsystem* UCompassKitStatics::GetCompassSubsystemForPlayer(const APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return nullptr;
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer)
    {
        return nullptr;
    }

    return LocalPlayer->GetSubsystem<UCompassLocalPlayerSubsystem>();
}