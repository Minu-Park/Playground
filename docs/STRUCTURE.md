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
| `CMakeLists.txt` | Playground repo | Host app CMake entry |
| `src` | Playground repo | Host app source |
| `docs` | Playground repo | Parent project docs |
| `AGENTS.md` | Playground repo | Always-read operating rules |
| `build` | ignored | Parent CMake build directories |

## Current App
- `src/main.cpp` creates one `QMainWindow`.
- `src/main.cpp` initializes shared styles and assets using `Resources::installResources`.
- `GraphicsEngine` is the central widget.
- `Camera` is controlled through `QCameraWidget` inside a right-side `QDockWidget`.
- `Gocator` is controlled through `QGocatorWidget` inside a right-side `QDockWidget`.
- 2D camera grabs convert `CPylonImage` to `QImage`, then call `GraphicsEngine::setImage`.
- 3D camera grabs convert `CPylonDataContainer` through `BlazeScene3DAdapter`, then call `GraphicsEngine::setScene3D`.
- Gocator grabs convert `GoPxLSdk::GoDataSet` through `GocatorDataSetScene3DAdapter`, then call `GraphicsEngine::setScene3D`.
- Device callbacks originate outside the GUI thread. UI updates are queued onto `GraphicsEngine`.
- `Camera` owns camera lifecycle and grabbing. The host app compiles `BlazeScene3DAdapter` for the current Camera + blaze integration path.
- `Gocator` owns discovery/connect/parameter/grabbing. The host app compiles `GocatorDataSetScene3DAdapter` for the current Gocator + GoPxL integration path.

## Boundaries
- Parent app may link module targets and call public module APIs.
- Parent app must not copy module implementation files.
- Device runtime details stay in `Camera`/`Gocator`.
- Rendering and neutral display contracts stay in `GraphicsEngine`.
- QSS, icons, and brand selectors stay in `Resources`, which provides `Resources::installResources` to initialize the application style.
- Sensor SDK adapters that expose SDK payload types are compiled by host apps that use those SDK modules.
- pylon is required for the current Camera + blaze integration path.
- GoPxL SDK is required for the current Gocator integration path.
- If module source changes are needed, change that module repo and report its git status separately.
