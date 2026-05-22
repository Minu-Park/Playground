# Imaging Controller Plan

## Purpose
- Move imaging lifecycle ownership out of `DeviceWindow` in staged, testable steps.
- Preserve the existing Camera `ready()` permit flow.
- Prepare image processing for multiple functions, dynamic libraries, and long-term OpenCV/PCL pipeline growth.
- Keep Gocator lifecycle refactoring deferred until its acquisition/backpressure contract is explicitly defined.

## Current Problem
- `DeviceWindow` currently owns too many roles:
  - Hosts `GraphicsEngine`.
  - Hosts device control widgets.
  - Registers Camera and Gocator callbacks.
  - Converts SDK payloads through host-side adapters.
  - Queues display updates to the GUI thread.
  - Contains the early OpenCV hook.
- Cause: acquisition, processing, rendering, and UI hosting are all colocated.
- Effect: adding multiple processing functions, external libraries, PCL steps, or pipeline UI will turn `DeviceWindow` into the lifecycle and processing bottleneck.

## Existing Camera Flow
- Camera grabbing already has a useful flow-control contract.
- `Camera::ready()` adds one permit.
- The grab thread waits for a permit before delivering the next non-trigger frame.
- The current host callback processes/converts the frame, queues a `GraphicsEngine` update, then calls `camera->ready()`.
- This must remain the default live-imaging policy.

## Target Ownership

```text
MainWindow
  creates imaging sessions through a factory
  owns workspace chrome, menus, logs, and shutdown ordering

DeviceWindow
  hosts the view and control surfaces for one session
  does not own acquisition or processing policy

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
- `DeviceWindow` owns only widget hosting and layout.
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

### Stage 1: Documented Boundary
- Add this plan.
- Update structure and development guide references.
- No code movement.
- Verification: `git diff --check`.

### Stage 2: Camera & Gocator ImagingController Skeleton
- Define `AbstractImagingController` interface.
- Add `CameraImagingController` and `GocatorImagingController` inheriting from it.
- Move Camera and Gocator callback registration and deregistration out of `DeviceWindow`.
- Keep current behavior exactly:
  - same `ready()` timing for camera,
  - same display routing.
- `DeviceWindow` instantiates `std::unique_ptr<AbstractImagingController>` and hosts it.
- Verification: configure/build and open/close Camera and Gocator windows.

### Stage 3: GraphicsEngineSink Extraction
- Move GUI-thread display enqueue into a small `GraphicsEngineSink` object.
- Keep `GraphicsEngine` public API unchanged.
- Connect both `CameraImagingController` and `GocatorImagingController` to `GraphicsEngineSink` for rendering updates.
- Verification: 2D image and Blaze/Gocator 3D still route through `setImage` and `setScene3D`.

### Stage 4: Pass-Through Pipeline
- Add `ProcessingFrame`, `ProcessingPipeline`, and pass-through node support.
- Replace direct conversion-to-display with conversion-to-frame, pipeline run, then sink.
- No dynamic libraries yet. Keep OpenCV optional and compile-time guarded.
- Verification: output must match Stage 3 behavior.

### Stage 5: ProcessingRegistry
- Add registry for built-in node definitions.
- Represent node metadata and parameter schema.
- Build a pipeline from registered definitions.
- Verification: pass-through pipeline still builds and runs.

### Stage 6: Dynamic Library Loader (Safe Hot-Swapping)
- Add cross-platform loader abstraction.
- Implement memory-safe, reference-counted library unloading (using `std::shared_ptr` or atomic locks) to prevent background thread segfaults during hot-swaps.
- Register exported node definitions into `ProcessingRegistry`.
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
