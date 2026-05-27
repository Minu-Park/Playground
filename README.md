# Playground

Playground is a Qt host workspace for composing and testing the Camera, Gocator, GraphicsEngine, and Resources modules.

## Purpose
- Create, remove, compose, and test module integrations.
- Keep module implementation ownership in `modules/*`.
- Keep host orchestration, MDI workspace behavior, and integration docs in the parent repo.
- Use `modules/Resources` for shared Qt resources, QSS, icons, and brand assets.

## Current App
- `MainWindow` owns the MDI workspace, device creation menu, global log dock, and shutdown ordering.
- `DeviceSession` owns one imaging session authority boundary.
- `GraphicsEngine` is the session central widget and remains visualization-only.
- The host installs the `GraphicsEngine` QVTK/OpenGL default surface format before `QApplication`, so first session creation does not perform delayed global graphics setup.
- Device control widgets are docked panels managed by the session.
- `QProcessingWidget` is hosted in a hidden-by-default `Image Processing Pipeline` dock.
- `CameraImagingController`, `GocatorImagingController`, and `StaticImageImagingController` own acquisition/session flow.
- `GraphicsEngineSink` queues display updates back to the GUI thread.
- Static image sessions can be created without hardware and populated from disk.
- Hiding a device control panel does not stop acquisition; explicit stop/remove actions own that lifecycle change.
- Camera and Gocator control widgets share the Resources status contract: `Idle`, `Disconnected`, `Connected`, and `Live`.
- `QGocatorWidget` reports grab transitions and submitted parameter edits through its status-bar message field; parameter text reflects submission until the module exposes apply results.
- `Gocator::syslog()` flushes each operation record so redirected lifecycle output reaches the host log dock promptly.
- Test Image controls use compact neutral button, FPS, and selection styling from `modules/Resources`.

## Modules
The parent repo tracks the modules as git submodules, but each module keeps its own git history and must be checked, changed, committed, and pushed separately.

| Path | Role |
| --- | --- |
| `modules/Camera` | Basler camera runtime and Qt camera control widget |
| `modules/GraphicsEngine` | Reusable Qt/VTK visualization library |
| `modules/Gocator` | LMI Gocator runtime and Qt control widget |
| `modules/Resources` | Shared qrc, QSS, icons, and app assets |

## Build
```bash
git clone --recurse-submodules git@github.com:minu-park/Playground.git
cd Playground

cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
./build/cmake-build-debug/Playground
```

If modules are missing after clone:
```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## Docs
- `AGENTS.md`: operating rules.
- `docs/STRUCTURE.md`: current ownership and integration layout.
- `docs/DEVELOPMENT_GUIDE.md`: working commands, validation loop, and priorities.
- `docs/NEXT_SESSION_HANDOFF.md`: active published baseline and next-session work order.
- `docs/SESSION_ARCHITECTURE.md`: class-by-class session ownership boundary.
- `docs/STEREO_3D_CAMERA_INTEGRATION.md`: current Stereo 3D contract and remaining decisions.
- `docs/DYNAMIC_PROCESSING_STATUS.md`: dynamic processing state and unresolved boundary.
- `docs/STRUCTURAL_REVIEW.md`: current structural risks and decisions needed.
