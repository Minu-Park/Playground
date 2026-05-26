# Playground Development Guide

## Read Order
1. `AGENTS.md`
2. `docs/NEXT_SESSION_HANDOFF.md` when it exists
3. `docs/STRUCTURE.md`
4. `docs/STRUCTURAL_REVIEW.md`
5. Task-relevant module docs

## Integration Status
- `docs/NEXT_SESSION_HANDOFF.md`: 게시 기준 커밋, 실장치 확인 결과, 다음 세션 착수 순서.
- `docs/STEREO_3D_CAMERA_INTEGRATION.md`: Basler Stereo mini/Stereo ace 현재 계약, 구현 결과, 남은 결정.

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

## Linux Runtime Linkage
- The host executable retains its explicit Qt OpenGL link on Linux because VTK reaches that library through a transitive runtime dependency.
- Verify a built executable with `ldd build/cmake-build-debug/Playground | rg 'not found|Qt6OpenGL'`.

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
- Camera 3D routing uses a pylon family profile and `PylonScene3DAdapter`: Blaze compatibility, Stereo mini direct XYZ/color mapping, and Stereo ace disparity/calibration conversion compile in the baseline.
- `GraphicsEngine` neutral scene contract preserves optional color images, registration metadata, and organized RGB for color point clouds.
- `Resources` owns reusable QSS and assets.
- `QGocatorWidget` reports callback-confirmed grab transitions and asynchronous parameter update submissions in its local status bar.
- `Gocator::syslog()` flushes operation records for immediate host-side redirected logging.
- Test Image control sizing and neutral selection styling remain in `modules/Resources/Style.qss`.
- OpenCV is optional; when CMake finds it, `HAS_OPENCV` enables the current live compilation UI. Dynamic ABI and cross-platform compiler policy remain unresolved.

## Current Priority
- Keep Camera/Gocator lifecycle cleanup stable on window close and app quit.
- Preserve Camera `ready()` based live-frame admission.
- Validate Basler Stereo mini and Stereo ace hardware against the implemented color Scene3D baseline, then add IR/profile UX and any capability gaps defined in `docs/STEREO_3D_CAMERA_INTEGRATION.md`.
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
