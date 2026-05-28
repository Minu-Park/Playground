# Playground Development Guide

## Read Order
1. `AGENTS.md`
2. `docs/STRUCTURE.md`
3. `docs/SESSION_ARCHITECTURE.md`
4. `docs/STRUCTURAL_REVIEW.md`
5. Task-relevant module docs

## Integration Status
- `docs/STEREO_3D_CAMERA_INTEGRATION.md`: Basler Stereo mini/Stereo ace 현재 계약, 구현 결과, 남은 결정.
- `docs/STRUCTURAL_REVIEW.md`: 처리 파이프라인 포함 현재 책임 경계 부채와 결정 대기 항목.

## Normal Checks
```bash
git status --short --branch
git -C modules/Camera status --short --branch
git -C modules/GraphicsEngine status --short --branch
git -C modules/Gocator status --short --branch
git -C modules/Resources status --short --branch
```

## Remote Freshness
- Local status alone does not prove that a checkout matches the remote. Fetch each repository before comparing commits.
- A `0 0` result means the checked-out `HEAD` matches the fetched upstream reference. Detached submodule `HEAD` is normal when the parent pins that commit.
- All four submodules must remain at their fetched upstream tip. A non-zero behind count requires updating the checked-out module commit and parent pointer, followed by parent integration validation.
- Run this check at work start and again before committing or publishing the parent repository.

```bash
git fetch origin
git -C modules/Camera fetch origin
git -C modules/GraphicsEngine fetch origin
git -C modules/Gocator fetch origin
git -C modules/Resources fetch origin

git rev-list --left-right --count HEAD...origin/main
git -C modules/Camera rev-list --left-right --count HEAD...origin/HEAD
git -C modules/GraphicsEngine rev-list --left-right --count HEAD...origin/HEAD
git -C modules/Gocator rev-list --left-right --count HEAD...origin/HEAD
git -C modules/Resources rev-list --left-right --count HEAD...origin/HEAD
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
- `MainWindow` creates a transparent one-pixel OpenGL composition seed before first show to avoid first-session insertion of the top-level window's first OpenGL child.
- `DeviceSession` owns per-session authority, controller lifetime, device control docks, processing docks, and display sink binding.
- `GraphicsEngine` is the session central widget and remains visualization-only.
- Device control dock visibility must not change acquisition state.
- Camera, Gocator, and Static Image sessions are routed through imaging controllers.
- `GraphicsEngineSink` owns GUI-thread display enqueue and explicit display clear.
- Camera 3D routing uses a pylon family profile and `PylonScene3DAdapter`: Blaze compatibility, Stereo mini direct XYZ/color mapping, and Stereo ace disparity/calibration conversion compile in the baseline.
- `GraphicsEngine` neutral scene contract preserves optional color images, registration metadata, and organized RGB for color point clouds.
- `Resources` owns reusable QSS and assets.
- `QGocatorWidget` reports callback-confirmed grab transitions and asynchronous parameter update submissions in its local status bar.
- `Gocator::syslog()` flushes operation records for immediate host-side redirected logging.
- `LogManager` writes `lastlog.log` under the app data location and appends each line instead of rewriting the rolling UI buffer.
- Test Image control sizing and neutral selection styling remain in `modules/Resources/Style.qss`.
- OpenCV is optional for the host build. `QProcessingWidget` always builds, while `DynamicProcessingCompiler` resolves OpenCV compile paths from configured runtime paths, app-local runtime paths, or CMake-discovered defaults.

## Runtime Paths
- Default behavior prefers system-discovered SDK paths when available.
- Runtime overrides are allowed for deployment and machine-specific SDK layouts.
- Users can edit OpenCV overrides from the settings button beside the OpenCV filter script compile button.
- OpenCV live compilation path resolution checks, in order: `QSettings`, environment variables, app-local runtime folders, then CMake defaults.
- OpenCV compiler path must point to a C++ compiler driver such as `clang++`, `g++`, or `c++`; C drivers such as `clang` do not link the C++ runtime.
- OpenCV `QSettings` keys:
  - `RuntimePaths/OpenCV/CompilerPath`
  - `RuntimePaths/OpenCV/IncludeDir`
  - `RuntimePaths/OpenCV/LibraryDir`
  - `RuntimePaths/OpenCV/Libraries`
- OpenCV environment variables:
  - `PLAYGROUND_OPENCV_COMPILER`
  - `PLAYGROUND_OPENCV_INCLUDE_DIR`
  - `PLAYGROUND_OPENCV_LIB_DIR`
  - `PLAYGROUND_OPENCV_LIBS`
- App-local OpenCV candidates:
  - `<appDir>/runtime/opencv/include`
  - `<appDir>/runtime/opencv/lib`
  - `<appDir>/runtime/OpenCV/include`
  - `<appDir>/runtime/OpenCV/lib`

## Current Priority
- Keep Camera/Gocator lifecycle cleanup stable on window close and app quit.
- OpenGL composition seed validation passed on macOS for first-session host refresh removal; repeat on Linux and monitor for visible seed artifacts.
- Preserve Camera `ready()` based live-frame admission.
- Validate Basler Stereo mini and Stereo ace hardware against the implemented color Scene3D baseline, then add IR/profile UX and any capability gaps defined in `docs/STEREO_3D_CAMERA_INTEGRATION.md`.
- Keep module changes inside each module repo and report module git status separately.
- Follow the implementation checklist in `docs/STRUCTURAL_REVIEW.md` before broadening the processing pipeline.
- Validate UI and lifecycle changes with the smallest viable parent configure/build.

## Next Structural Sequence
1. Generalize runtime path policy for pylon and GoPxL/Gocator SDKs.
2. Add explicit runtime diagnostics UI/logging for missing pylon, GoPxL, and OpenCV paths.
3. Document `process_image` ABI v1 inputs, ownership, channel format, and compatibility rules.
4. Decide whether multi-node processing uses ABI v1 as a per-node wrapper or needs a new plugin ABI.
5. Add manual platform validation notes for macOS/Linux/Windows OpenCV compile/load paths.

## Imaging Controller Work Order
1. Keep `AbstractImagingController`, `CameraImagingController`, `GocatorImagingController`, and `StaticImageImagingController` as the acquisition/session boundary.
2. Keep GUI-thread display enqueue in `GraphicsEngineSink`.
3. Keep `DeviceSession` as the owner of session composition and widget layout.
4. Keep `ProcessingPipeline` pass-through-safe before adding expensive processing.
5. Keep the current single `process_image` hot-swap contract until a multi-node processing contract is defined.
6. Extract compile/load policy from the widget before claiming cross-platform live compilation.
7. Extend the pipeline UI only after the backend can represent multiple node instances.
8. Extend 3D processing only after Scene3D admission and memory policy are explicit.

## Rules
- Parent code belongs in `src`.
- Module code belongs in `modules/<Name>`.
- Parent build directories stay under `build/`.
- Keep every submodule current with its fetched upstream and check each repo independently before final reporting.
- Publish changed module commits first; publish the parent pointer only after all referenced module commits are available upstream and the parent validation passes.
- Keep callback UI writes queued to the GUI thread.
- Keep style changes in `modules/Resources` unless a widget needs an object-name/API change.
- Ask before changing module ownership, callback admission policy, render visibility policy, or backpressure behavior.

## Fresh Clone Troubleshooting
- If CMake reports that a module has no `CMakeLists.txt`, the submodule worktree is not checked out.
- Run `git submodule sync --recursive`, then `git submodule update --init --recursive`.
- Reconfigure under `build/cmake-build-debug` after module checkout is correct.
