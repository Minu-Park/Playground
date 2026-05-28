# Playground Structure

## Purpose
- Playground is the parent workspace for composing, testing, adding, and removing modules.
- Modules keep their own source ownership and git history.
- The parent app wires modules together without absorbing module internals.

## Repository Model
- `modules/*` are git submodules in the parent repo.
- Each module is also an independent repository with separate history.
- Parent commits should record host code, docs, and submodule pointers only.
- Module implementation changes must be committed in the touched module repo first.
- Each submodule pointer must track the latest fetched upstream commit used by the parent integration; a stale module pointer is not an accepted steady state.
- If a module upstream advances, the parent updates that pointer only after the new module set passes integration validation.

## Layout
| Path | Owner | Role |
| --- | --- | --- |
| `.gitmodules` | Playground repo | Module checkout manifest |
| `CMakeLists.txt` | Playground repo | Host app CMake entry |
| `src` | Playground repo | Host orchestration source |
| `docs` | Playground repo | Parent project docs |
| `AGENTS.md` | Playground repo | Always-read operating rules |
| `build` | ignored | Parent CMake build directories |
| `modules/Camera` | Camera repo | Basler 2D/blaze/Stereo mini/Stereo ace runtime, pylon callbacks, Scene3D adapters, Qt camera control widget |
| `modules/GraphicsEngine` | GraphicsEngine repo | Reusable Qt/VTK visualization widget library |
| `modules/Gocator` | Gocator repo | LMI Gocator runtime, discovery, grabbing, and Qt control widget |
| `modules/Resources` | Resources repo | Shared Qt qrc bundle, icons, single-theme app stylesheet, brand selectors, and reusable chrome widgets |
| `modules/Resources/Chrome/MainTitleBar.*` | Resources repo | Frameless top-level title bar, embedded menu, and host window controls |
| `modules/Resources/Chrome/CustomTitleBar.*` | Resources repo | Per-session MDI title bar and embedded session menu |
| `modules/Resources/Chrome/DockTitleBar.*` | Resources repo | Shared custom title bar for host/session docks |
| `modules/Resources/Chrome/MdiSubWindowContainer.*` | Resources repo | Generic MDI wrapper for styled frame, resize hit testing, minimized/maximized presentation, and injected content |
| `modules/Resources/theme/qss/*.qss` | Resources repo | Ordered stylesheet parts loaded by `Resources::installResources` |

## Current App
- `src/main.cpp` installs the `GraphicsEngine` QVTK/OpenGL default surface format before `QApplication`, then initializes `LogManager`, installs shared Resources through `Resources::installResources(app)`, and launches `MainWindow`.
- `MainWindow` owns a transparent one-pixel `QOpenGLWidget` composition seed under the MDI viewport before the host is shown; this moves first top-level OpenGL composition setup out of session addition without pre-creating a `GraphicsEngine`.
- `MainWindow` hosts a `QMdiArea` central workspace.
- `MainWindow` is a frameless `QMainWindow`; `Resources` provides `MainTitleBar`, which embeds the menu bar, app title, logo, and minimize/maximize/close controls.
- `MainWindow` owns the outer resize hit testing because the native frame is disabled.
- The MDI viewport is painted by the host app with neutral gray `#eeeeee` and `:/Resources/BASLER_Logo.png`.
- Users can add Basler Camera, LMI Gocator, or Test Image sessions as MDI subwindows.
- Sessions are added as frameless `QMdiSubWindow` instances containing the Resources `MdiSubWindowContainer`; the generic wrapper owns resize/minimized/maximized presentation and uses injected session content plus the Resources `CustomTitleBar`.
- The Window menu tiles visible, non-minimized MDI subwindows by their current spatial order so left-to-right placement stays predictable.
- `MainWindow` creates `DeviceSession` subwindows and deletes them before `CameraSystem` destruction so device callbacks and camera ownership are cleaned up in order.
- `LogManager` captures Qt logs plus redirected module `std::cout` and `std::cerr` logs into the System Logs dock and an app-data `lastlog.log`; `Gocator::syslog()` flushes each operation record for prompt forwarding.
- `RuntimeDependencyResolver` provides app-level path injection for runtime/build dependencies such as OpenCV, with system discovery as the default fallback.
- Camera and Gocator status labels use the shared Resources `status` property map for `Idle`, `Disconnected`, `Connected`, and `Live`.
- Camera and Gocator message labels use a `messageState` property; Resources owns normal/error colors when the host installs the theme.
- `QGocatorWidget` keeps operation feedback local to its status bar: grab messages follow requested and callback-confirmed transitions, while parameter messages identify asynchronous update submissions.
- Top-level title bars, session title bars, MDI wrappers, dock title bars, split chrome styling, and title-button icons live in `modules/Resources`.
- Test Image tool buttons, FPS input, list selection, and GraphicsEngine line-profile helper controls use compact neutral styling owned by `modules/Resources`.

## Device Session
- `DeviceSession` is the per-session authority boundary.
- `GraphicsEngine` is the session central widget.
- `DeviceSession` receives its wrapper `QMdiSubWindow` pointer only for size/layout coordination; the wrapper owns chrome and hit testing, while the session keeps lifecycle and routing authority.
- The session control widget lives in a docked control panel:
  - `QCameraWidget` for Basler Camera.
  - `QGocatorWidget` for LMI Gocator.
  - `QStaticImageControlWidget` for offline image playback.
- `QProcessingWidget` lives in a right-side `Image Processing Pipeline` dock, hidden by default, and exposes one hot-swappable OpenCV script node with parsed parameters through compact icon buttons.
- The `View` menu exposes text dock toggles for the control and processing panels.
- Hiding a control or processing panel does not stop acquisition.
- `GraphicsEngineSink` queues display and explicit clear calls to the local `GraphicsEngine`.

## Imaging Controllers
- `AbstractImagingController` defines the session lifecycle interface.
- `CameraImagingController` owns Camera callback registration, `PylonScene3DProfile`-aware `PylonScene3DAdapter` invocation, pipeline execution, display sink binding, and Camera `ready()` timing.
- `GocatorImagingController` owns Gocator callback registration, pipeline execution, and display sink binding.
- `StaticImageImagingController` owns file list playback, FPS timing, pipeline execution, and display sink binding.
- `ProcessingPipeline` owns ordered processing node instances.
- `QProcessingWidget` owns the currently exposed single `process_image` hot-swap node UI and opens the OpenCV runtime path dialog from the filter script toolbar.
- `DynamicProcessingCompiler` owns generated source, compiler arguments, output artifact paths, and the OpenCV build environment for the hot-swap node.
- `DynamicLibraryLoader` keeps the installed processing library mapped while frames can invoke it.
- `QGocatorWidget` owns feature-value refresh and parameter-update watchers so background work is cancelled or joined during shutdown.

## Data Flow
- Camera 2D frames convert from pylon image payloads to `QImage`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setImage`.
- Camera Blaze 3D frames route through `PylonScene3DAdapter` to the existing Blaze conversion path, then enqueue as `GraphicsScene3D`.
- Camera Stereo mini color 3D frames preserve Source3 RGB display and associate it to direct `Range/Coord3D_ABC32f` geometry for point colors using the official sample mapping rule.
- Camera Stereo ace color 3D frames use `Intensity/RGB8` plus `Disparity/Coord3D_C16` and captured calibration values to produce neutral scene data.
- `GraphicsScene3D` can carry a color image and registration metadata; organized `RangeFrame::rgb` feeds color point-cloud generation without storing live point-cloud buffers in the engine state.
- Gocator data sets convert through `GocatorDataSetScene3DAdapter`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setScene3D`.
- Static images load from disk as `QImage`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setImage`; removing the last image queues an explicit display clear.
- Device callbacks originate outside the GUI thread. Display updates must stay queued to the GUI thread.

## Boundaries
- Parent app may link module targets and call public module APIs.
- Parent app must not copy generic module implementation code.
- Device runtime details stay in `Camera` and `Gocator`.
- Rendering and neutral display contracts stay in `GraphicsEngine`.
- Session composition and lifecycle authority stay in `DeviceSession`.
- Shared theme QSS, icons, and brand selectors stay in `Resources`.
- Styled title bars, MDI wrapper, button assets, and theme QSS parts live in `modules/Resources`; parent `src/UI` keeps only host-specific controls.
- Shared device status and message colors stay in `Resources`; device widgets may set semantic properties but must not hardcode the shared palette.
- Module code must not call `Resources::installResources`; host applications install the theme, while modules keep default Qt fallback behavior for standalone use.
- Host-only workspace chrome, MDI behavior, and session composition stay in `src`.
- SDK-specific adapters that expose SDK payload types are compiled by host apps that use those SDK modules.
- pylon is required for Camera paths; Stereo mini and Stereo ace require their installed pylon 3D Supplementary Package/producer at runtime.
- GoPxL SDK is required for the current Gocator integration path.
- The host and GraphicsEngine public integration contract use a C++17 compilation baseline.
- Processing pipeline changes must not bypass the Camera `ready()` flow-control contract.

## Known Structural Issues
See `docs/STRUCTURAL_REVIEW.md`.
