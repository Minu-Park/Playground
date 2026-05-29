# Session Architecture

## Decision
- `DeviceSession` is the parent-owned session authority.
- `GraphicsEngine` is the central session surface.
- Native top-level and MDI frames are replaced by Resources chrome widgets assembled by the parent app.
- Device, processing, and display routing are composed by `DeviceSession`.
- Device control panels may be hidden without stopping acquisition.

## Cause -> Effect
- Cause: device control widgets can be closed or hidden while an acquisition/rendering session should remain active.
- Effect: device control widget lifetime must not own hardware lifetime.
- Cause: acquisition, processing, and display routing share one session lifecycle.
- Effect: `MainWindow` should create sessions, but `DeviceSession` should own session composition.
- Cause: `GraphicsEngine` is a reusable visualization module.
- Effect: it must not learn Camera, Gocator, or processing orchestration concepts.
- Cause: frameless windows remove native title bars and resize handles.
- Effect: `MainWindow`, `MdiSubWindowContainer`, and title-bar widgets own chrome input handling while `DeviceSession` stays the lifecycle owner.

## Class Boundaries

| Class | Responsibility | Must Not Own |
| --- | --- | --- |
| `MainWindow` | App shell, Resources-provided frameless top-level chrome, MDI workspace, menus, global logs, global statusbar summaries, `CameraSystem`, startup OpenGL composition seed, session creation/removal ordering | Frame routing, processing policy, per-device UI internals |
| `modules/Resources/Chrome` widgets | `MainTitleBar`, `CustomTitleBar`, `MdiSubWindowContainer`, and `DockTitleBar` reusable chrome presentation/input widgets | Device/session lifecycle, acquisition state, frame routing |
| `DockTitleBar` | Styled dock title and dock close/float controls provided by Resources | Dock content behavior, acquisition state |
| `DeviceSession` | One session authority, device lifetime binding, controller lifetime, processing dock, control dock, display sink binding, wrapper-size coordination | Module internals, generic rendering implementation, chrome hit testing |
| `GraphicsEngine` | Central visualization widget, 2D/3D display state, render interaction | Camera/Gocator lifecycle, processing graph, session policy |
| `AbstractImagingController` | Common start/stop/grab-state contract | Widget layout, app shell menus |
| `CameraImagingController` | Camera callback registration, `Camera::ready()` admission, `PylonScene3DProfile` 기반 2D/3D conversion, pipeline execution, display enqueue | `MainWindow`, dock layout |
| `GocatorImagingController` | Gocator callback registration, conversion, pipeline execution, display enqueue | `MainWindow`, dock layout |
| `StaticImageImagingController` | Offline image list playback, FPS timing, pipeline execution, display enqueue | File picker UI layout |
| `GraphicsEngineSink` | Queued GUI-thread delivery and explicit display clear to the session `GraphicsEngine` | Acquisition policy, processing policy |
| `QCameraWidget` / `QGocatorWidget` / `QStaticImageControlWidget` | Device or source control panel UI | Hardware lifetime authority |
| `QProcessingWidget` | Pipeline editing UI, parameter controls, compile request dispatch | Frame admission, render visibility policy, compiler/link argument policy |
| `RuntimePathsDialog` | User-facing runtime path override editing | Dependency discovery order, compile/load execution |
| `DynamicProcessingCompiler` | Generated source, compile arguments, output artifact paths, OpenCV build environment | UI layout, frame processing state |

## Status Contract
- Main statusbar shows global app state only: session count, active session type, and OpenCV runtime path mode.
- Camera and Gocator control widgets expose status through widget-specific labels and the shared dynamic property `status`.
- `Resources::installResources(app)` owns the shared style map for `Idle`, `Disconnected`, `Connected`, and `Live`.
- Camera and Gocator message labels expose `messageState`; Resources owns normal/error message appearance when installed by the host.
- Source widgets may decide when a connection attempt has happened.
- `QGocatorWidget` owns its transient operation message: grab completion text follows `GrabbingStatus`, while parameter-edit text reports an asynchronous submission until the API exposes an apply-result signal.
- Source widgets must not hardcode the shared status or message palette.

## Widget Layout

```text
MainWindow
  owns QMdiArea
  uses Resources MainTitleBar
  owns CameraSystem
  creates frameless QMdiSubWindow wrappers

QMdiSubWindow
  widget: Resources MdiSubWindowContainer
  title row: Resources CustomTitleBar
  body: DeviceSession

DeviceSession
  central widget: GraphicsEngine
  left dock: source control panel with Resources DockTitleBar
  right dock: Image Processing Pipeline with Resources DockTitleBar
  owns: device pointer binding, controller, sink
```

## Lifecycle Policy
- Closing a `DeviceSession` ends that session.
- Hiding the source control dock only hides controls.
- Hiding the processing dock only hides processing controls.
- Stopping acquisition must happen through a session/device action, not dock visibility.
- Minimizing, maximizing, dragging, or resizing the wrapper changes presentation only.
- Deleting a Camera session removes the camera from `CameraSystem` after controller teardown.
- Deleting a Gocator session closes and deletes the Gocator after controller teardown.
- Camera sessions must preserve `Camera::ready()` as the live-frame admission contract.

## Routing Policy
- Controllers convert device payloads into `ProcessingFrame`.
- Camera 3D routing keeps Blaze compatibility and converts Stereo mini/Stereo ace mono/color inputs through the Camera module's pylon Scene3D adapter.
- `ProcessingPipeline` runs before display enqueue.
- `GraphicsEngineSink` delivers processed output and explicit clear requests to the session `GraphicsEngine` on the GUI thread.
- `GraphicsEngine` remains unaware of source type.

## Naming Policy
- Use `DeviceSession` for the authority object.
- Use `Control Panel` for user-facing dock text.

## Current Deferrals
- Hidden-viewer retention policy is no longer tied to a dock.
- Scene3D processing capability is still policy-limited until Scene3D nodes exist.
- Dynamic OpenCV compilation uses `process_image` ABI v1. OpenCV paths can come from runtime settings, app-local runtime folders, or CMake-discovered defaults; broader plugin ABI remains unresolved.
