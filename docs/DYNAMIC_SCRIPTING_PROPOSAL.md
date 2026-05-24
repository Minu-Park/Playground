# Dynamic Processing Proposal

## Purpose
- Let developers test image-processing code inside Playground without rebuilding the host app.
- Keep acquisition, processing, and rendering boundaries explicit.
- Support C++ dynamic libraries first, with OpenCV optional and PCL/3D deferred.

## Current State
- `QProcessingWidget` exists in the parent app.
- `ProcessingPipeline` and `ProcessingRegistry` exist.
- `DynamicLibraryLoader` exists and keeps the library mapped while dynamic node wrappers are alive.
- OpenCV activation in `CMakeLists.txt` is currently commented out.
- The UI can show processing controls, but OpenCV live compilation is inactive unless `HAS_OPENCV` is restored.
- A Qt-only built-in `Invert Image` node exists as a basic 2D pipeline test node.

## Target Model
```text
QProcessingWidget
  edits or loads processing code
  reports compiler/load errors
  adds node instances to ProcessingPipeline

ProcessingRegistry
  owns available node definitions
  creates node instances

ProcessingPipeline
  owns ordered node instances
  runs Image2D now
  runs Scene3D later

DynamicLibraryLoader
  loads shared libraries
  resolves exported node factories
  keeps libraries alive while nodes are active
```

## Dynamic Library Contract
- Dynamic nodes should export one stable factory symbol, currently `create_node`.
- The returned object must implement the host processing-node ABI expected by the parent app.
- Library unload must wait until no active pipeline node is executing code from that library.
- File suffixes must be selected by platform:
  - Windows: `.dll`
  - Linux: `.so`
  - macOS: `.dylib`

## OpenCV Direction
- OpenCV should stay optional.
- Re-enable the CMake block only when include paths, library paths, and runtime compiler flags are cross-platform.
- Do not hardcode Homebrew, `/opt`, or macOS-only compiler assumptions in the final path.
- The processing UI should report disabled OpenCV support clearly when `HAS_OPENCV` is absent.

## Backpressure Rule
- Camera live mode remains single-source-admission through `Camera::ready()`.
- Processing must not add an unbounded queue.
- On processing error, the controller must log, drop or pass through the frame, and still release the Camera permit.

## 3D Direction
- Scene3D/PCL processing is deferred.
- PCL compile time and memory cost make hot-compile behavior risky for live acquisition.
- 3D processing should start with built-in nodes before runtime compilation.
- Scene3D processing must preserve range/point-cloud semantics unless a node explicitly changes them.

## Decisions Needed
- Whether explicit clear-display semantics should allow an empty static-image session to clear the central viewer.
- Whether Gocator sessions should expose the same processing UI before Scene3D nodes exist.
- Whether dynamic C++ nodes are acceptable in-process, or should move to an out-of-process sandbox later.
