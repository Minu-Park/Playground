# Playground Development Guide

## Read Order
1. `AGENTS.md`
2. `docs/STRUCTURE.md`
3. Task-relevant module docs

## Commands
```bash
git pull
git submodule sync --recursive
git submodule update --init --recursive
git status --short --branch
git -C modules/Camera status --short --branch
git -C modules/GraphicsEngine status --short --branch
git -C modules/Gocator status --short --branch
git -C modules/Resources status --short --branch
cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
git diff --check
git -C modules/Camera diff --check
git -C modules/Gocator diff --check
git -C modules/Resources diff --check
```

## Current State
- The app is an MDI workspace. `MainWindow` owns device creation, global log dock wiring, MDI background painting, and shutdown ordering.
- `DeviceWindow` owns per-device control dock placement and callback routing into its local `GraphicsEngine`.
- `CameraSystem` owns Basler `Camera` instances. `MainWindow` must delete MDI subwindows before `CameraSystem` is destroyed.
- `Gocator` owns discovery, connection, configuration, grabbing, and `[Gocator]` stream logging.
- `Resources` owns reusable QSS and assets. The host app references `:/Resources/BASLER_Logo.png` for the MDI watermark.
- OpenCV runtime compilation code exists but is currently disabled in the active app flow.

## Current Priority
- Keep Camera/Gocator lifecycle cleanup stable on window close and app quit.
- Refactor Camera and Gocator imaging lifecycles in stages, using `docs/IMAGING_CONTROLLER_PLAN.md`.
- Preserve Camera `ready()` based live-frame admission while moving code.
- Keep module changes inside each module repo and commit module repos separately from the parent.
- Validate UI changes with the smallest viable parent configure/build.

## Imaging Controller Work Order
1. Define `AbstractImagingController` and implement `CameraImagingController` and `GocatorImagingController` skeletons.
2. Extract GUI-thread display enqueue into `GraphicsEngineSink`.
3. Add a pass-through `ProcessingPipeline` before adding processing functions. Keep OpenCV optional.
4. Add `ProcessingRegistry` before dynamic library loading.
5. Add dynamic library loading with reference-counted unloading safety.
6. Add pipeline UI after the backend can represent multiple node instances.
7. Add offline static image simulation pipeline (Add Image) to allow testing hot-swapped nodes without hardware (Completed).
8. Extend the pipeline to 3D processing (Scene3D / Point Cloud) (Deferred).

## Imaging Controller Rules
- Do not replace Camera `ready()` with an unbounded queue.
- Do not change `ready()` timing without a performance decision.
- Call `ready()` after processing and display enqueue, including processing-error paths.
- Treat one processing function, multiple functions, and dynamic libraries as the same node-definition/node-instance model.
- Keep dynamic library extensions behind a cross-platform loader abstraction.
- Guarantee dynamic library code segments are not currently in-flight on grab threads before calling `QLibrary::unload()` (e.g., using reference counting).

## Fresh Clone Troubleshooting
- If CMake reports that `modules/GraphicsEngine` or `modules/Resources` has no `CMakeLists.txt`, the submodule worktree is not checked out.
- From the Playground root, run `git pull`, `git submodule sync --recursive`, then `git submodule update --init --recursive`.
- Remove or reuse the existing `build/` directory only after module checkout is correct.

## Historical Plan: MDI Layout & OpenCV C++ Dynamic Compilation

To ensure compile-readiness and prevent context token bloat, the plan is broken down into atomic, testable sub-steps classified by difficulty (Easy, Medium, Hard).

---

### Milestone 1: MDI Frame & Custom Subwindows (Skeleton)
*   **Step 1.1 [Easy]**: Add placeholders for new source files (`src/MainWindow.h`, `src/MainWindow.cpp`, `src/DeviceWindow.h`, `src/DeviceWindow.cpp`) into `CMakeLists.txt`. Define empty stub classes.
    *   *Test*: Run CMake and verify it configures and compiles with the empty stubs.
*   **Step 1.2 [Easy]**: Implement the `MainWindow` class with a `QMdiArea` as central widget, menus/toolbars, and empty slot actions ("Add Basler Camera", "Add LMI Gocator"). Update `src/main.cpp` to instantiate it.
    *   *Test*: Run the app. Verify a blank MDI application window shows up.
*   **Step 1.3 [Medium]**: Implement `DeviceWindow` skeleton inheriting from `QMainWindow`. Its constructor instantiates a `GraphicsEngine` as the central widget, a right dock placeholder, and a bottom dock placeholder.
    *   *Test*: Run the app. Click toolbar actions to add windows. Verify subwindows spawn with `GraphicsEngine` in the center and empty docks on the sides.

---

### Milestone 2: Dynamic Basler Camera Integration
*   **Step 2.1 [Easy]**: Integrate `CameraSystem` in `MainWindow`. When clicking "Add Basler Camera", call `cameraSystem->addCamera()` and pass the `Camera*` to `DeviceWindow`.
    *   *Test*: Run and click "Add Basler Camera". Verify that camera allocation is logged without crashes.
*   **Step 2.2 [Medium]**: Inside `DeviceWindow` (Basler variant), instantiate `QCameraWidget(camera)` and add it to the right dock. Register a mock camera grab callback that logs frame sequences.
    *   *Test*: Open a camera window, toggle connection, click grab, and verify that the logs print without UI freezing.
*   **Step 2.3 [Medium]**: Implement the real 2D pylon image grab callback in `DeviceWindow`. Convert `CPylonImage` to `QImage` and call `GraphicsEngine::setImage` on the GUI thread using `QMetaObject::invokeMethod`.
    *   *Test*: Start grabbing and verify that the live video feed renders in the central `GraphicsEngine`.
*   **Step 2.4 [Easy]**: Implement the cleanup logic in `DeviceWindow::closeEvent` and destructor (deregister callbacks, stop grab, notify `CameraSystem` to remove the camera, and delete the camera pointer).
    *   *Test*: Open and close multiple Basler Camera subwindows. Verify that camera instances are deleted and pylon handles are closed cleanly.

---

### Milestone 3: Dynamic LMI Gocator Integration
*   **Step 3.1 [Easy]**: When clicking "Add LMI Gocator", allocate a new `Gocator` instance and construct `DeviceWindow(gocator)`.
    *   *Test*: Click "Add LMI Gocator". Verify gocator instance is created.
*   **Step 3.2 [Medium]**: Inside `DeviceWindow` (Gocator variant), instantiate `QGocatorWidget(gocator)` and add it to the right dock.
    *   *Test*: Click "Add LMI Gocator". Verify the control UI is displayed in the right dock.
*   **Step 3.3 [Hard]**: Register Gocator data callbacks in `DeviceWindow`. Convert datasets using `GocatorDataSetScene3DAdapter` to `GraphicsScene3D` and update the local `GraphicsEngine` via `setScene3D` on the GUI thread.
    *   *Test*: Connect to a mock/real Gocator, scan, and verify that the 3D range frame renders on the central `GraphicsEngine`.
*   **Step 3.4 [Easy]**: Implement Gocator cleanup in `DeviceWindow::closeEvent` and destructor (deregister callbacks, close Gocator connection, delete Gocator pointer).
    *   *Test*: Verify that opening and closing multiple Gocators cleans up connection states and deletes pointers.

---

### Milestone 4: CMake Conditional Dependency for OpenCV
*   **Step 4.1 [Easy]**: Modify `CMakeLists.txt` to find OpenCV, link libraries, and add the `HAS_OPENCV` compile definition if found.
    *   *Test*: Run CMake configure. Verify that it prints search status (found/not found) and builds without errors in both cases.
*   **Step 4.2 [Easy]**: Add pylon-to-OpenCV converter function (using `#ifdef HAS_OPENCV`) to `src/` to prepare for processing.
    *   *Test*: Verify compilation with and without OpenCV.

---

### Milestone 5: Runtime C++ Compilation Editor
*   **Step 5.1 [Medium]**: Create `QProcessingWidget` containing a `QTextEdit` editor (preloaded with a template OpenCV `processImage` snippet), a `Compile` button, parameter sliders, and a read-only log console. Add it to the bottom dock of the Basler `DeviceWindow`.
    *   *Test*: Build and verify the code panel renders correctly in the bottom area of the camera window.
*   **Step 5.2 [Hard]**: Implement `QProcess` compilation in `QProcessingWidget`. Save the text as `scratch/dynamic_filter.cpp` and run `clang++` to compile it to `scratch/libdynamic_filter.dylib` with OpenCV headers/libraries. Pipe stderr to the log console.
    *   *Test*: Write buggy code, click Compile, and verify that clang++ errors appear in red. Write correct code, click Compile, and verify "Compilation Succeeded!".

---

### Milestone 6: Live Image Processing Pipeline
*   **Step 6.1 [Medium]**: Implement `QLibrary` loading, unloading, and symbol resolution of the `processImage` function inside `QProcessingWidget`.
    *   *Test*: Verify that compilation loads the library and resolves the symbol address correctly.
*   **Step 6.2 [Hard]**: Hook the resolved function pointer into the camera grab loop: convert pylon image to `cv::Mat`, process it via the loaded function, convert the result back to `QImage`, and update `GraphicsEngine`. Add thread synchronization.
    *   *Test*: Run live grab. Write a Sobel/Canny filter, compile it. Verify the live feed changes to show the filtered image, and adjusting sliders updates the results in real-time.

## Rules
- Parent code belongs in `src`.
- Module code belongs in `modules/<Name>`.
- Parent build directories stay under `build/`.
- Check each repo independently before final reporting.
- Keep callback UI writes queued to the GUI thread.
- Keep style changes out of device/engine repos unless they are widget object-name/API changes.
- Ask before changing module ownership, callback admission policy, or render/backpressure behavior.
