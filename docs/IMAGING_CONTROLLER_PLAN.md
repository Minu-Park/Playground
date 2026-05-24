# Imaging Controller Plan

## Purpose
- Move imaging lifecycle ownership out of window chrome in staged, testable steps.
- Preserve the existing Camera `ready()` permit flow.
- Prepare image processing for multiple functions, dynamic libraries, and long-term OpenCV/PCL pipeline growth.
- Keep Gocator lifecycle refactoring deferred until its acquisition/backpressure contract is explicitly defined.

## Current Problem
- The controller boundary now exists, but frame-domain and viewer-visibility policy are still not settled.
- `DeviceSession` hosts the visual/control surfaces and owns controller lifetime.
- Camera/Gocator adapters still compile in the host because they expose SDK payload types.
- `QProcessingWidget` is available for sessions before Scene3D processing semantics are complete.
- Cause: acquisition/session ownership was extracted before processing-domain policy was finalized.
- Effect: UI can expose processing controls more broadly than the current Image2D-focused pipeline supports.

## Existing Camera Flow
- Camera grabbing already has a useful flow-control contract.
- `Camera::ready()` adds one permit.
- The grab thread waits for a permit before delivering the next non-trigger frame.
- `CameraImagingController` processes/converts the frame, queues a `GraphicsEngine` update, then calls `camera->ready()`.
- This must remain the default live-imaging policy.

## Target Ownership

```text
MainWindow
  creates imaging sessions through a factory
  owns workspace chrome, menus, logs, and shutdown ordering

DeviceSession
  owns one session authority boundary
  hosts the central GraphicsEngine and docked control surfaces
  owns controller lifetime
  owns acquisition-to-processing-to-display composition
  does not own module internals

AbstractImagingController
  base interface defining lifecycle contract (start, stop, isGrabbing)

CameraImagingController
  inherits AbstractImagingController
  owns Camera callback registration
  owns Camera lifecycle binding
  owns pipeline execution
  owns GraphicsEngine sink binding
  calls Camera::ready() after processing and display enqueue

GocatorImagingController
  inherits AbstractImagingController
  owns Gocator callback registration
  owns Gocator lifecycle binding
  owns GraphicsEngine sink binding

ProcessingRegistry
  owns available processing node definitions
  supports built-in and dynamic-library-provided nodes

ProcessingPipeline
  owns ordered node instances
  runs Image2D and, later, Scene3D processing

GraphicsEngineSink
  receives processed display payloads
  calls GraphicsEngine::setImage or GraphicsEngine::setScene3D on the GUI thread
```

## Boundaries
- `Camera` owns Basler/pylon acquisition and the `ready()` permit mechanism.
- `AbstractImagingController` defines the generic device acquisition lifecycle interface.
- `CameraImagingController` owns Camera-to-pipeline-to-view composition.
- `GocatorImagingController` owns Gocator-to-view composition.
- `ProcessingPipeline` owns processing order and node execution.
- `ProcessingRegistry` owns function/library discovery and node creation.
- `GraphicsEngine` owns rendering and display mode decisions.
- `DeviceSession` owns session composition, widget hosting, and layout.
- `MainWindow` owns only workspace-level creation, logs, and shutdown sequencing.
- Both Camera and Gocator are unified under their respective implementations of `AbstractImagingController`.

## Processing Model

### Function Definition
- Describes what processing is available.
- Includes stable id, display name, version, frame domain, input type, output type, and parameter schema.
- May come from built-in code or a dynamic library.

### Node Instance
- One configured use of a function definition inside a pipeline.
- Owns parameter values, enabled state, and instance id.
- Multiple instances may reference the same function definition with different parameters.

### Pipeline
- Ordered list of node instances.
- Runs each node in sequence.
- Starts as pass-through.
- Later supports multiple OpenCV nodes, dynamic library nodes, and PCL nodes.

### Executor
- Owns thread, cancellation, error handling, and live-frame admission.
- Live Camera mode remains single-in-flight by default.
- Processing errors drop the frame and log the error; they must not crash or stall the grab loop.

## Dynamic Library Direction
- Do not bind the live controller directly to one function pointer.
- Load dynamic libraries through a loader.
- Register exported processing definitions into `ProcessingRegistry`.
- Build pipeline node instances from registry definitions.
- Unload a library only after all active pipeline instances using its definitions are removed.
- Use platform-specific file extensions behind one abstraction:
  - Windows: `.dll`
  - Linux: `.so`
  - macOS: `.dylib`

## Frame Domains
- `Image2D`: Camera intensity/image path, OpenCV-first.
- `Scene3D`: Blaze/Gocator scene path, PCL or geometry-first later.
- Cross-domain conversion must be explicit and deferred.

## Backpressure Policy
- Preserve Camera `ready()` as the source-level flow-control contract.
- The controller calls `ready()` after:
  - frame conversion,
  - pipeline execution,
  - display update enqueue,
  - error handling if processing fails.
- Do not add unbounded queues.
- Do not change frame admission policy without an explicit performance decision.

## Staged Plan

### Stage 1: Documented Boundary (Completed)
- Add this plan.
- Update structure and development guide references.
- No code movement.
- Verification: `git diff --check`.

### Stage 2: Camera & Gocator ImagingController Skeleton (Completed)
- `AbstractImagingController`, `CameraImagingController`, and `GocatorImagingController` exist.
- Camera and Gocator callback registration moved out of window-level code.
- `DeviceSession` owns `std::unique_ptr<AbstractImagingController>`.
- Camera `ready()` timing remains in the Camera controller.
- Verification: configure/build and open/close Camera and Gocator windows.

### Stage 2b: DeviceSession Authority Boundary (Completed)
- `DeviceWindow` was renamed to `DeviceSession`.
- `GraphicsEngine` is the central session widget.
- Camera, Gocator, and static-image control widgets moved to docked control panels.
- Hiding control panels no longer implies a session lifecycle change.
- Verification: configure/build and open/close sessions.

### Stage 3: GraphicsEngineSink Extraction (Completed)
- GUI-thread display enqueue lives in `GraphicsEngineSink`.
- `GraphicsEngine` public API remains unchanged.
- Camera, Gocator, and Static Image controllers bind to the sink.
- Verification: 2D image and Blaze/Gocator 3D still route through `setImage` and `setScene3D`.

### Stage 4: Pass-Through Pipeline (Completed)
- `ProcessingFrame` and `ProcessingPipeline` exist.
- Controllers convert to frame, run the pipeline, then enqueue through the sink.
- OpenCV remains optional and compile-time guarded.
- Verification: output must match Stage 3 behavior.

### Stage 5: ProcessingRegistry (Started)
- `ProcessingRegistry` exists.
- A built-in `Invert Image` node exists.
- Node metadata remains minimal.
- Verification: pass-through pipeline still builds and runs.

### Stage 6: Dynamic Library Loader (Started)
- `DynamicLibraryLoader` exists.
- Dynamic node wrappers keep the loader alive while node objects exist.
- Cross-platform compiler activation and live hot-swap validation remain incomplete.
- Verification: load/unload sample library during active grab thread without crashes.

### Stage 7: Pipeline UI
- Replace single-function processing UI assumptions with:
  - node list,
  - per-node enable/disable,
  - parameter editor,
  - compile/load logs.
- Verification: changing node parameters must not block the grab thread.

### Stage 8: Offline Test Image Simulation Pipeline (Completed)
- Implement `StaticImageImagingController` and `QStaticImageControlWidget` to support loading files from disk.
- Allow developers to test OpenCV C++ hot-swapped nodes offline using multi-image loop playback, prev/next frame navigation, and adjustable FPS.
- Verification: Create a test image session from the menu, add images, play/pause, adjust FPS, and apply the pipeline without physical camera hardware.

### Stage 9: PCL / Scene3D Processing (Deferred)
- Extend the same registry/pipeline model to `Scene3D` (Point Cloud / RangeFrame).
- Verification: no point-cloud or range data loss unless an explicit processing node requests it.

## Deferred
- Gocator backpressure or single-in-flight admission.
- Full-sequence processing mode.
- Save/replay pipeline.
- Cross-domain conversion between `Image2D` and `Scene3D`.
- Hidden-viewer render retention policy.
