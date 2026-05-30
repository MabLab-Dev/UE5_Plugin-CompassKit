# `UE5 Plugin` CompassKit

CompassKit is an Unreal Engine 5 plugin that provides an in-game compass and navigation marker system for tracking objectives, actors, and directions at runtime.

It supports dynamic actor tracking, cardinal direction markers, and widget-agnostic navigation workflows while remaining lightweight, extensible, and blueprint-friendly.

> [!NOTE]
> CompassKit is designed to be a widget-agnostic navigation system that manages marker state and provides runtime projection data for different navigation widget implementations.
>
> The current release focuses on compass bar widgets, with future updates planned for screen-space indicators and additional navigation systems.

## Features

- Dynamic runtime marker system
- Moving actor tracking
- Cardinal direction markers
- Point/world location markers
- LocalPlayerSubsystem architecture
- Blueprint-friendly API
- Runtime projection helpers
- Automatic cleanup for destroyed actors
- Example gameplay map included
- Example compass bar widget included

## Demo

[YouTube Link](https://www.youtube.com/watch?v=IHyVL0eF_3Q)

https://github.com/user-attachments/assets/21239d7d-6f84-40b4-89d1-19e13401641c

## Quick Start

### 1. Add the Compass Widget

Add the provided `WBP_CompassBar` widget to your player's existing HUD widget.

### 2. Identify the Player and Target Actor

Markers are added for a specific player and track a target actor or world location.

To create a marker, you need:
- the player controller that will see the marker
- the actor or location to track


### 3. Get the Compass Subsystem and Add a marker

Create a `CompassMarkerData` struct describing the target actor or world location, then call `AddMarker` from the player's compass subsystem.

<img width="65%" height="65%" alt="CompassKitV1_UsageExample" src="https://github.com/user-attachments/assets/81c3321e-f41f-4f8e-8284-2c4ca71ce2ff" />

## Marker Types

### Cardinal Markers

Directional markers independent of world position.

Example uses:
- North / East / South / West
- Directional indicators

### Point Markers

Track:
- Moving actors
- Static world locations

Point markers are updated automatically at runtime.

> [!NOTE]
> Support for area markers is planned for future updates.

## Projection Modes

### Perspective_3D

Uses directional projection for a more spatial compass feel.

<img width="50%" height="50%" alt="CompassKitV1_Perspective3d" src="https://github.com/user-attachments/assets/83825906-ad55-4f1f-91c8-9cab58075672" />

### Flat_2D

Uses direct angular mapping for a traditional flat compass feel.

<img width="50%" height="50%" alt="CompassKitV1_Flat2D" src="https://github.com/user-attachments/assets/55ee39bc-1604-48a3-b928-511daceea488" />

## Example Content

The plugin includes an example gameplay map demonstrating:

- Dynamic marker spawning
- Multiple simultaneous markers
- Moving actor tracking
- Automatic marker cleanup
- Runtime add/remove workflows

The example gameplay consists of physics-based balls being spawned into the world while the player tracks and catches them using the compass system.

## Included Assets

<img width="100%" height="100%" alt="CompassKitV1_AssetList" src="https://github.com/user-attachments/assets/526acc3e-c71c-432d-af33-74ab5381037c" />

## Architecture Overview

CompassKit uses a LocalPlayerSubsystem to manage marker state and runtime navigation data. Navigation widgets subscribe to the subsystem and generate their own marker widgets using runtime query functions.

<img width="25%" height="25%" alt="CompassKitV1_ArchitectureDiagram" src="https://github.com/user-attachments/assets/a96d0f7d-4ebf-4c0f-8dac-0fbb31cb3d26" />

## Blueprint API Overview

### Widget Subscription

A navigation widget must subscribe in order to receive marker updates.

- `SubscribeNavigationWidget`
- `UnsubscribeNavigationWidget`

### Marker Lifecycle

- `AddMarker`
- `UpdateMarker`
- `RemoveMarker`
- `ClearMarkers`

### Runtime Queries

These runtime query functions are designed to support custom navigation widgets beyond the included compass bar implementation.

- `GetMarkerRuntimeCache`
- `GetMarkerDistance`
- `GetMarkerProjectedOffset`
- `IsMarkerWithinView`

### Marker Widget Interface

CompassKit includes a `ICompassMarkerVisualInterface` for marker widgets, to simplify communication between a navigation widget and its marker widget implementations.

Included interface functions:

- `InitializeFromMarkerData`
- `UpdateDistance`

This allows custom marker widgets to react to runtime marker data without tightly coupling widget implementations to the subsystem.

## Notes

- The markers are added/removed for a specific player hence are managed by a local player subsystem
- The subsystem uses tick to continuously update marker positions, but tick is only active if a navigation widget is subscribed and there are markers to be tracked.
- CompassKit is designed to encourage custom navigation widget implementations.

## Installation

1. Copy the plugin into your project's `Plugins` folder
2. Enable the plugin in Unreal Engine
3. Restart the editor
4. Play the example map `Lvl_CompassKitTest`

## Requirements

- Unreal Engine 5.7

> [!NOTE]
> This initial release targets Unreal Engine 5.7 only. Backward compatibility for earlier UE5 versions will be added in future updates.

## License

This project is licensed under the [MIT License](LICENSE).

## Support

Bug reports, feature requests, and general feedback are welcome through the [GitHub Issues](../../issues) page.
