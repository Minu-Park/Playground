# Implementation Walkthrough

## Current Baseline
- Playground is an MDI host app.
- Device sessions are created from the `Device` menu.
- The System Logs dock receives Qt logs and redirected module stream logs.
- Shared styling and icons are installed through `Resources::installResources(app)`.
- Device sessions route acquisition through imaging controllers and display through `GraphicsEngineSink`.

## Session Types
| Session | Control Widget | Controller | Display |
| --- | --- | --- | --- |
| Basler Camera | `QCameraWidget` | `CameraImagingController` | `GraphicsEngine` in `Live Viewer` dock |
| LMI Gocator | `QGocatorWidget` | `GocatorImagingController` | `GraphicsEngine` in `Live Viewer` dock |
| Test Images | `QStaticImageControlWidget` | `StaticImageImagingController` | `GraphicsEngine` in `Live Viewer` dock |

## Processing
- `QProcessingWidget` is available in a hidden-by-default dock.
- `ProcessingPipeline` runs ordered nodes against `ProcessingFrame`.
- `ProcessingRegistry` registers built-in and dynamic node definitions.
- `DynamicLibraryLoader` can load a dynamic node factory.
- OpenCV live compilation is currently disabled by CMake comments.

## Lifecycle
- `MainWindow` owns `CameraSystem`.
- `MainWindow` deletes MDI subwindows before `CameraSystem` destruction.
- `DeviceWindow` destroys its controller before removing/deleting the device object.
- Camera controllers preserve `Camera::ready()` timing after processing and display enqueue.

## Verification Checklist
```bash
git status --short --branch
git -C modules/Camera status --short --branch
git -C modules/GraphicsEngine status --short --branch
git -C modules/Gocator status --short --branch
git -C modules/Resources status --short --branch
cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
git diff --check
```
