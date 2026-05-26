# Playground Development Guide

## Read Order
1. `AGENTS.md`
2. `docs/STRUCTURE.md`
3. `docs/STRUCTURAL_REVIEW.md`
4. Task-relevant module docs

## Normal Checks
```bash
git status --short --branch
git -C modules/Camera status --short --branch
git -C modules/GraphicsEngine status --short --branch
git -C modules/Gocator status --short --branch
git -C modules/Resources status --short --branch
```

## Module Checkout
```bash
git submodule sync --recursive
git submodule update --init --recursive
```

## Build
```bash
cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
```

## Diff Checks
```bash
git diff --check
git -C modules/Camera diff --check
git -C modules/GraphicsEngine diff --check
git -C modules/Gocator diff --check
git -C modules/Resources diff --check
```

## Current State
- The app is an MDI workspace.
- `MainWindow` owns device creation, global log dock wiring, MDI background painting, and shutdown ordering.
- `DeviceSession` owns per-session authority, controller lifetime, device control docks, processing docks, and display sink binding.
- `GraphicsEngine` is the session central widget and remains visualization-only.
- Device control dock visibility must not change acquisition state.
- Camera, Gocator, and Static Image sessions are routed through imaging controllers.
- `GraphicsEngineSink` owns GUI-thread display enqueue.
- `Resources` owns reusable QSS and assets.
- `QGocatorWidget` reports callback-confirmed grab transitions and asynchronous parameter update submissions in its local status bar.
- `Gocator::syslog()` flushes operation records for immediate host-side redirected logging.
- Test Image control sizing and neutral selection styling remain in `modules/Resources/Style.qss`.
- OpenCV CMake activation is commented out; dynamic processing UI is present but OpenCV live compilation is inactive unless `HAS_OPENCV` is restored.

## Current Priority
- Keep Camera/Gocator lifecycle cleanup stable on window close and app quit.
- Preserve Camera `ready()` based live-frame admission.
- Keep module changes inside each module repo and report module git status separately.
- Resolve the structural review items before broadening the processing pipeline.
- Validate UI and lifecycle changes with the smallest viable parent configure/build.

## Imaging Controller Work Order
1. Keep `AbstractImagingController`, `CameraImagingController`, `GocatorImagingController`, and `StaticImageImagingController` as the acquisition/session boundary.
2. Keep GUI-thread display enqueue in `GraphicsEngineSink`.
3. Keep `DeviceSession` as the owner of session composition and widget layout.
4. Keep `ProcessingPipeline` pass-through-safe before adding expensive processing.
5. Add or load processing nodes through `ProcessingRegistry`.
6. Re-enable dynamic library loading only with cross-platform compiler, library suffix, and unload safety.
7. Extend the pipeline UI only after the backend can represent multiple node instances.
8. Extend 3D processing only after Scene3D admission and memory policy are explicit.

## Rules
- Parent code belongs in `src`.
- Module code belongs in `modules/<Name>`.
- Parent build directories stay under `build/`.
- Check each repo independently before final reporting.
- Keep callback UI writes queued to the GUI thread.
- Keep style changes in `modules/Resources` unless a widget needs an object-name/API change.
- Ask before changing module ownership, callback admission policy, render visibility policy, or backpressure behavior.

## Fresh Clone Troubleshooting
- If CMake reports that a module has no `CMakeLists.txt`, the submodule worktree is not checked out.
- Run `git submodule sync --recursive`, then `git submodule update --init --recursive`.
- Reconfigure under `build/cmake-build-debug` after module checkout is correct.
