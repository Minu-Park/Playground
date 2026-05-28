# Playground

Playground is a Qt host workspace for composing and testing the Camera, Gocator, GraphicsEngine, and Resources modules.

## Purpose
- Create, remove, compose, and test module integrations.
- Keep module implementation ownership in `modules/*`.
- Keep host orchestration, MDI workspace behavior, and integration docs in the parent repo.
- Use `modules/Resources` for shared Qt resources, single-theme split QSS, chrome widgets, icons, and brand assets.

## Current App
- `MainWindow` owns the MDI workspace, device creation menu, global log dock, and shutdown ordering.
- `MainWindow` now uses Resources-provided frameless top-level chrome with an embedded menu row, custom window controls, and host-owned resize hit testing.
- `DeviceSession` owns one imaging session authority boundary.
- Each session is wrapped by the Resources-provided `MdiSubWindowContainer` and `CustomTitleBar` so the MDI chrome can be reused without moving session lifecycle authority out of `DeviceSession`.
- `GraphicsEngine` is the session central widget and remains visualization-only.
- The host installs the `GraphicsEngine` QVTK/OpenGL default surface format before `QApplication`, so first session creation does not perform delayed global graphics setup.
- `MainWindow` adds a transparent one-pixel OpenGL composition seed before its first show, so adding the first session does not introduce the top-level window's first OpenGL child on platforms affected by Qt's dynamic `QOpenGLWidget` insertion behavior.
- Device control widgets are docked panels managed by the session and use the Resources-provided `DockTitleBar` for the custom dock chrome.
- `QProcessingWidget` is hosted in a hidden-by-default `Image Processing Pipeline` dock; `DynamicProcessingCompiler` owns the OpenCV hot-swap compile plan and can use runtime paths configured from the compact icon toolbar rather than requiring the host app to link OpenCV.
- `CameraImagingController`, `GocatorImagingController`, and `StaticImageImagingController` own acquisition/session flow.
- `GraphicsEngineSink` queues display updates back to the GUI thread.
- Static image sessions can be created without hardware and populated from disk.
- Hiding a device control panel does not stop acquisition; explicit stop/remove actions own that lifecycle change.
- Camera and Gocator control widgets share the Resources status contract: `Idle`, `Disconnected`, `Connected`, and `Live`.
- Camera and Gocator message labels expose only `messageState`; Resources owns the normal/error message styling when installed by the host.
- `QGocatorWidget` reports grab transitions and submitted parameter edits through its status-bar message field; parameter text reflects submission until the module exposes apply results.
- `QGocatorWidget` serializes parameter edits and feature-value refreshes through owned watchers so UI refresh and shutdown do not leave anonymous background watchers running.
- `Gocator::syslog()` flushes each operation record so redirected lifecycle output reaches the host log dock promptly.
- Test Image controls and GraphicsEngine line-profile helper controls use compact neutral styling from `modules/Resources`.
- `GraphicsEngine` 2D image views support Ctrl-wheel manual zoom around the cursor while preserving the fit-mode toggle contract.

## Modules
The parent repo tracks the modules as git submodules, but each module keeps its own git history and must be checked, changed, committed, and pushed separately.
Every module is kept current with its fetched upstream tip. Before parent publication, fetch and compare all module repositories, update any advanced submodule pointer, and verify the parent integration against the resulting set of module commits.

| Path | Role |
| --- | --- |
| `modules/Camera` | Basler camera runtime and Qt camera control widget |
| `modules/GraphicsEngine` | Reusable Qt/VTK visualization library |
| `modules/Gocator` | LMI Gocator runtime and Qt control widget |
| `modules/Resources` | Shared qrc, single-theme split QSS, reusable chrome widgets, icons, and app assets |

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
- `docs/SESSION_ARCHITECTURE.md`: class-by-class session ownership boundary.
- `docs/STEREO_3D_CAMERA_INTEGRATION.md`: current Stereo 3D contract and remaining decisions.
- `docs/STRUCTURAL_REVIEW.md`: current structural risks and decisions needed.
