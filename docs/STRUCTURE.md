# Playground Structure

## Purpose
- Playground is the parent workspace for composing, testing, adding, and removing modules.
- Modules keep their own source ownership and git history.
- Parent code wires modules together without absorbing module internals.

## Layout
| Path | Owner | Role |
| --- | --- | --- |
| `modules/Camera` | Camera repo | Basler camera runtime, Qt camera control widget, pylon callbacks |
| `modules/GraphicsEngine` | GraphicsEngine repo | Reusable Qt/VTK visualization widget library |
| `modules/Gocator` | Gocator repo | LMI Gocator runtime and UI library |
| `modules/Resources` | Resources repo | Shared Qt qrc bundle, icons, app stylesheet, brand selectors |
| `.gitmodules` | Playground repo | Submodule URL manifest for fresh clone initialization |
| `CMakeLists.txt` | Playground repo | Host app CMake entry |
| `src` | Playground repo | Host app source |
| `docs` | Playground repo | Parent project docs |
| `AGENTS.md` | Playground repo | Always-read operating rules |
| `build` | ignored | Parent CMake build directories |

## Current App
- `src/main.cpp` initializes shared styles and assets using `Resources::installResources`, registers the global `LogManager`, and launches `MainWindow`.
- `MainWindow` hosts a `QMdiArea` central widget.
- The MDI viewport is painted by the host app with neutral gray `#eeeeee` and a centered half-size `:/Resources/BASLER_Logo.png` watermark.
- Users can dynamically add cameras/sensors as MDI subwindows (`DeviceWindow`).
- Each `DeviceWindow` pairs visualization and control:
  - Central Widget: `GraphicsEngine` for primary visual focus.
  - Right Dock: `QCameraWidget` or `QGocatorWidget` for device settings.
  - Bottom Dock: `QProcessingWidget` for runtime C++ OpenCV code editing and compilation (Note: OpenCV features are currently disabled).
- 2D camera grabs are routed through the runtime OpenCV compiled filter (if active and enabled) and displayed in the central `GraphicsEngine` via `setImage`.
- 3D data from Gocator or Blaze is adapter-converted and set via `setScene3D` on the local `GraphicsEngine`.
- Device callbacks originate outside the GUI thread. UI updates are queued onto the local `GraphicsEngine` instance.
- `Camera` owns camera lifecycle and grabbing. The host app compiles `BlazeScene3DAdapter` for the current Camera + blaze integration path.
- `Gocator` owns discovery/connect/parameter/grabbing. The host app compiles `GocatorDataSetScene3DAdapter` for the current Gocator + GoPxL integration path.
- `MainWindow` explicitly deletes MDI subwindows before `CameraSystem` destruction so `DeviceWindow` can deregister callbacks and remove cameras while the owning system is still alive.
- **Global Logging**: A global thread-safe logger (`LogManager`) intercepts Qt logs and redirects module `std::cout`/`std::cerr` logs, writing them to a rolling log file `lastlog.log` (capped at 500 lines) and mirroring them to the "System Logs" `QDockWidget` at the bottom of the `MainWindow` via `Qt::QueuedConnection`.
- `CameraSystem::syslog()` emits `[Camera System]` stream logs, and `Gocator::syslog()` emits `[Gocator]` stream logs for discovery, connection, configuration, grabbing, stopping, and warning events.

## Boundaries
- Parent app may link module targets and call public module APIs.
- Parent app must not copy module implementation files.
- Device runtime details stay in `Camera`/`Gocator`.
- Rendering and neutral display contracts stay in `GraphicsEngine`.
- QSS, icons, and brand selectors stay in `Resources`, which provides `Resources::installResources` to initialize the application style.
- Host-only workspace chrome such as MDI background painting stays in `src`; reusable assets stay in `Resources`.
- Sensor SDK adapters that expose SDK payload types are compiled by host apps that use those SDK modules.
- pylon is required for the current Camera + blaze integration path.
- GoPxL SDK is required for the current Gocator integration path.
- If module source changes are needed, change that module repo and report its git status separately.
