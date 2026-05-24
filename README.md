# Playground

Playground is a Qt host workspace for composing and testing the Camera, Gocator, GraphicsEngine, and Resources modules.

## Purpose
- Create, remove, compose, and test module integrations.
- Keep module implementation ownership in `modules/*`.
- Keep host orchestration, MDI workspace behavior, and integration docs in the parent repo.
- Use `modules/Resources` for shared Qt resources, QSS, icons, and brand assets.

## Current App
- `MainWindow` owns the MDI workspace, device creation menu, global log dock, and shutdown ordering.
- `DeviceWindow` owns one imaging session shell.
- The device control widget is the session central widget.
- `GraphicsEngine` is hosted in a toggleable `Live Viewer` dock.
- `QProcessingWidget` is hosted in a hidden-by-default `Image Processing Pipeline` dock.
- `CameraImagingController`, `GocatorImagingController`, and `StaticImageImagingController` own acquisition/session flow.
- `GraphicsEngineSink` queues display updates back to the GUI thread.
- Static image sessions can be created without hardware and populated from disk.

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
- `docs/IMAGING_CONTROLLER_PLAN.md`: staged imaging lifecycle plan.
- `docs/DYNAMIC_SCRIPTING_PROPOSAL.md`: dynamic processing direction and current constraints.
- `docs/STRUCTURAL_REVIEW.md`: current structural risks and decisions needed.
