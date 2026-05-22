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

CameraImagingController
  owns Camera callback registration
  owns Camera lifecycle binding
  owns pipeline execution
  owns GraphicsEngine sink binding
  calls Camera::ready() after processing and display enqueue

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
- `CameraImagingController` owns Camera-to-pipeline-to-view composition.
- `ProcessingPipeline` owns processing order and node execution.
- `ProcessingRegistry` owns function/library discovery and node creation.
- `GraphicsEngine` owns rendering and display mode decisions.
- `DeviceWindow` owns only widget hosting and layout.
- `MainWindow` owns only workspace-level creation, logs, and shutdown sequencing.
- Gocator remains on the current `DeviceWindow` path until a matching backpressure or admission policy is chosen.

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

### Stage 2: CameraImagingController Skeleton
- Add a parent-owned `CameraImagingController`.
- Move Camera callback registration and deregistration out of `DeviceWindow`.
- Keep current behavior exactly:
  - same `ready()` timing,
  - same `QMetaObject::invokeMethod` display enqueue,
  - same Camera 2D and Blaze 3D adapter use.
- `DeviceWindow` still creates and hosts the controller.
- Verification: configure/build and open/close Camera windows.

### Stage 3: GraphicsEngineSink Extraction
- Move GUI-thread display enqueue into a small sink object.
- Keep `GraphicsEngine` public API unchanged.
- Verification: 2D image and Blaze 3D still route through `setImage` and `setScene3D`.

### Stage 4: Pass-Through Pipeline
- Add `ProcessingFrame`, `ProcessingPipeline`, and pass-through node support.
- Replace direct conversion-to-display with conversion-to-frame, pipeline run, then sink.
- No dynamic libraries yet.
- Verification: output must match Stage 3 behavior.

### Stage 5: ProcessingRegistry
- Add registry for built-in node definitions.
- Represent node metadata and parameter schema.
- Build a pipeline from registered definitions.
- Verification: pass-through pipeline still builds and runs.

### Stage 6: Dynamic Library Loader
- Add cross-platform loader abstraction.
- Register exported node definitions into `ProcessingRegistry`.
- Do not enable live hot-swap until unload safety is implemented.
- Verification: load/unload sample library outside live grab first.

### Stage 7: Pipeline UI
- Replace single-function processing UI assumptions with:
  - node list,
  - per-node enable/disable,
  - parameter editor,
  - compile/load logs.
- Verification: changing node parameters must not block the grab thread.

### Stage 8: PCL / Scene3D Processing
- Extend the same registry/pipeline model to `Scene3D`.
- Keep Camera Blaze and Gocator policies separate until Gocator admission is defined.
- Verification: no point-cloud or range data loss unless an explicit processing node requests it.

## Deferred
- Gocator lifecycle controller.
- Gocator backpressure or single-in-flight admission.
- Full-sequence processing mode.
- Save/replay pipeline.
- Cross-domain conversion between `Image2D` and `Scene3D`.
