# Session Architecture

## Decision
- `DeviceSession` is the parent-owned session authority.
- `GraphicsEngine` is the central session surface.
- Device, processing, and display routing are composed by `DeviceSession`.
- Device control panels may be hidden without stopping acquisition.

## Cause -> Effect
- Cause: device control widgets can be closed or hidden while an acquisition/rendering session should remain active.
- Effect: device control widget lifetime must not own hardware lifetime.
- Cause: acquisition, processing, and display routing share one session lifecycle.
- Effect: `MainWindow` should create sessions, but `DeviceSession` should own session composition.
- Cause: `GraphicsEngine` is a reusable visualization module.
- Effect: it must not learn Camera, Gocator, or processing orchestration concepts.

## Class Boundaries

| Class | Responsibility | Must Not Own |
| --- | --- | --- |
| `MainWindow` | App shell, MDI workspace, menus, global logs, `CameraSystem`, startup OpenGL composition seed, session creation/removal ordering | Frame routing, processing policy, per-device UI internals |
| `DeviceSession` | One session authority, device lifetime binding, controller lifetime, processing dock, control dock, display sink binding | Module internals, generic rendering implementation |
| `GraphicsEngine` | Central visualization widget, 2D/3D display state, render interaction | Camera/Gocator lifecycle, processing graph, session policy |
| `AbstractImagingController` | Common start/stop/grab-state contract | Widget layout, app shell menus |
| `CameraImagingController` | Camera callback registration, `Camera::ready()` admission, `PylonScene3DProfile` 기반 2D/3D conversion, pipeline execution, display enqueue | `MainWindow`, dock layout |
| `GocatorImagingController` | Gocator callback registration, conversion, pipeline execution, display enqueue | `MainWindow`, dock layout |
| `StaticImageImagingController` | Offline image list playback, FPS timing, pipeline execution, display enqueue | File picker UI layout |
| `GraphicsEngineSink` | Queued GUI-thread delivery to the session `GraphicsEngine` | Acquisition policy, processing policy |
| `QCameraWidget` / `QGocatorWidget` / `QStaticImageControlWidget` | Device or source control panel UI | Hardware lifetime authority |
| `QProcessingWidget` | Pipeline editing UI | Frame admission, render visibility policy |

## Status Contract
- Camera and Gocator control widgets expose status through widget-specific labels and the shared dynamic property `status`.
- `Resources::installResources(app)` owns the shared style map for `Idle`, `Disconnected`, `Connected`, and `Live`.
- Source widgets may decide when a connection attempt has happened.
- `QGocatorWidget` owns its transient operation message: grab completion text follows `GrabbingStatus`, while parameter-edit text reports an asynchronous submission until the API exposes an apply-result signal.
- Source widgets must not hardcode the shared status palette.

## Widget Layout

```text
MainWindow
  owns QMdiArea
  owns CameraSystem
  creates DeviceSession subwindows

DeviceSession
  central widget: GraphicsEngine
  left dock: source control panel
  right dock: Image Processing Pipeline
  owns: device pointer binding, controller, sink
```

## Lifecycle Policy
- Closing a `DeviceSession` ends that session.
- Hiding the source control dock only hides controls.
- Hiding the processing dock only hides processing controls.
- Stopping acquisition must happen through a session/device action, not dock visibility.
- Deleting a Camera session removes the camera from `CameraSystem` after controller teardown.
- Deleting a Gocator session closes and deletes the Gocator after controller teardown.
- Camera sessions must preserve `Camera::ready()` as the live-frame admission contract.

## Routing Policy
- Controllers convert device payloads into `ProcessingFrame`.
- Camera 3D routing keeps Blaze compatibility and converts Stereo mini/Stereo ace mono/color inputs through the Camera module's pylon Scene3D adapter.
- `ProcessingPipeline` runs before display enqueue.
- `GraphicsEngineSink` delivers processed output to the session `GraphicsEngine` on the GUI thread.
- `GraphicsEngine` remains unaware of source type.

## Naming Policy
- Use `DeviceSession` for the authority object.
- Use `Control Panel` for user-facing dock text.

## Current Deferrals
- Hidden-viewer retention policy is no longer tied to a dock, but explicit clear-display semantics remain unresolved.
- Scene3D processing capability is still policy-limited until Scene3D nodes exist.
- Dynamic OpenCV compilation is enabled only when OpenCV is found; its ABI and cross-platform compiler/output policy remain unresolved.
