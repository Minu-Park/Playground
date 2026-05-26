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

## Layout
| Path | Owner | Role |
| --- | --- | --- |
| `.gitmodules` | Playground repo | Module checkout manifest |
| `CMakeLists.txt` | Playground repo | Host app CMake entry |
| `src` | Playground repo | Host orchestration source |
| `docs` | Playground repo | Parent project docs |
| `AGENTS.md` | Playground repo | Always-read operating rules |
| `build` | ignored | Parent CMake build directories |
| `modules/Camera` | Camera repo | Basler camera runtime, pylon callbacks, Qt camera control widget |
| `modules/GraphicsEngine` | GraphicsEngine repo | Reusable Qt/VTK visualization widget library |
| `modules/Gocator` | Gocator repo | LMI Gocator runtime, discovery, grabbing, and Qt control widget |
| `modules/Resources` | Resources repo | Shared Qt qrc bundle, icons, app stylesheet, brand selectors |

## Current App
- `src/main.cpp` creates `QApplication`, initializes `LogManager`, installs shared Resources through `Resources::installResources(app)`, and launches `MainWindow`.
- `MainWindow` hosts a `QMdiArea` central workspace.
- The MDI viewport is painted by the host app with neutral gray `#eeeeee` and `:/Resources/BASLER_Logo.png`.
- Users can add Basler Camera, LMI Gocator, or Test Image sessions as MDI subwindows.
- The Window menu tiles visible, non-minimized MDI subwindows by their current spatial order so left-to-right placement stays predictable.
- `MainWindow` creates `DeviceSession` subwindows and deletes them before `CameraSystem` destruction so device callbacks and camera ownership are cleaned up in order.
- `LogManager` captures Qt logs plus redirected module `std::cout` and `std::cerr` logs into the System Logs dock and `lastlog.log`; `Gocator::syslog()` flushes each operation record for prompt forwarding.
- Camera and Gocator status labels use the shared Resources `status` property map for `Idle`, `Disconnected`, `Connected`, and `Live`.
- `QGocatorWidget` keeps operation feedback local to its status bar: grab messages follow requested and callback-confirmed transitions, while parameter messages identify asynchronous update submissions.
- Test Image tool buttons, FPS input, and list selection use compact neutral styling owned by `modules/Resources`.

## Device Session
- `DeviceSession` is the per-session authority boundary.
- `GraphicsEngine` is the session central widget.
- The session control widget lives in a docked control panel:
  - `QCameraWidget` for Basler Camera.
  - `QGocatorWidget` for LMI Gocator.
  - `QStaticImageControlWidget` for offline image playback.
- `QProcessingWidget` lives in an `Image Processing Pipeline` dock, hidden by default.
- The `View` menu exposes text dock toggles for the control and processing panels.
- Hiding a control or processing panel does not stop acquisition.
- `GraphicsEngineSink` queues display calls to the local `GraphicsEngine`.

## Imaging Controllers
- `AbstractImagingController` defines the session lifecycle interface.
- `CameraImagingController` owns Camera callback registration, pipeline execution, display sink binding, and Camera `ready()` timing.
- `GocatorImagingController` owns Gocator callback registration, pipeline execution, and display sink binding.
- `StaticImageImagingController` owns file list playback, FPS timing, pipeline execution, and display sink binding.
- `ProcessingPipeline` owns ordered processing node instances.
- `ProcessingRegistry` owns available built-in and dynamic processing node definitions.
- `DynamicLibraryLoader` owns dynamic library lifetime for loaded processing nodes.

## Data Flow
- Camera 2D frames convert from pylon image payloads to `QImage`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setImage`.
- Camera Blaze 3D frames convert through `BlazeScene3DAdapter`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setScene3D`.
- Gocator data sets convert through `GocatorDataSetScene3DAdapter`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setScene3D`.
- Static images load from disk as `QImage`, pass through `ProcessingPipeline`, then enqueue to `GraphicsEngine::setImage`.
- Device callbacks originate outside the GUI thread. Display updates must stay queued to the GUI thread.

## Boundaries
- Parent app may link module targets and call public module APIs.
- Parent app must not copy generic module implementation code.
- Device runtime details stay in `Camera` and `Gocator`.
- Rendering and neutral display contracts stay in `GraphicsEngine`.
- Session composition and lifecycle authority stay in `DeviceSession`.
- Shared QSS, icons, and brand selectors stay in `Resources`.
- Shared device status colors stay in `Resources`; device widgets may set status properties but must not hardcode the shared palette.
- Host-only workspace chrome, MDI behavior, and session composition stay in `src`.
- SDK-specific adapters that expose SDK payload types are compiled by host apps that use those SDK modules.
- pylon is required for the current Camera and Blaze integration path.
- GoPxL SDK is required for the current Gocator integration path.
- Processing pipeline changes must not bypass the Camera `ready()` flow-control contract.

## Known Structural Issues
See `docs/STRUCTURAL_REVIEW.md`.
