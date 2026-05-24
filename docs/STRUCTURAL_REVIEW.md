# Structural Review

## Scope
- Review date: 2026-05-24.
- Scope: parent Playground repo plus observed module status.
- Code was not changed in this review.
- Items below are cause -> effect findings.

## Git State
- Parent repo has uncommitted host code changes in `src`.
- `modules/Resources` has a modified `Style.qss` and an untracked `.DS_Store`.
- `modules/Camera`, `modules/GraphicsEngine`, and `modules/Gocator` are clean at their current submodule pointers.

## Findings

### 1. Viewer Visibility Now Controls Rendering
- Cause: `GraphicsEngine` was moved from the central widget into a toggleable `Live Viewer` dock, and `GraphicsEngineSink` now checks `target->isVisible()` before calling `setImage` or `setScene3D`.
- Effect: hidden viewer docks drop incoming display updates instead of retaining latest state.
- Risk: reopening the viewer can show stale content, and live Camera/Gocator sessions may keep acquiring while rendering is suppressed.
- Decision needed: choose whether hidden viewers should keep latest-frame state or intentionally pause/drop display updates.

### 2. Empty Image Clears Are Blocked
- Cause: `GraphicsEngineSink::enqueueImage` returns early for null images.
- Effect: `StaticImageImagingController::removeImage` cannot clear the viewer after the last file is removed because it enqueues `QImage()`.
- Risk: stale image content can remain visible after the session becomes empty.
- Decision needed: add an explicit clear-display API, or allow null-image clear semantics in the sink.

### 3. Session Layout Policy Changed
- Cause: `DeviceWindow` now uses the control widget as the central widget and docks `GraphicsEngine`.
- Effect: docs and mental model must treat the live viewer as optional chrome instead of the primary central surface.
- Risk: future feature work may attach processing or status behavior to the wrong widget area.
- Decision needed: confirm this control-first layout as the target before more UI work.

### 4. Processing UI Is Broader Than Processing Capability
- Cause: `QProcessingWidget` is created for Camera, Gocator, and Static Image sessions, while the active built-in processing coverage is mainly Image2D and OpenCV compilation is disabled.
- Effect: Gocator users can see a processing panel before Scene3D processing semantics are defined.
- Risk: UI suggests 3D processing capability that is not structurally ready.
- Decision needed: hide or disable the processing dock for Scene3D sessions until Scene3D nodes exist, or implement explicit Scene3D pass-through status.

### 5. Dynamic Compilation Is Partially Wired
- Cause: `QProcessingWidget`, `ProcessingRegistry`, and `DynamicLibraryLoader` are present, but OpenCV CMake activation is commented out.
- Effect: runtime compilation is visible as a direction but not an active baseline feature.
- Risk: stale assumptions about OpenCV paths, library suffixes, and compiler availability can re-enter the live path.
- Decision needed: either keep it explicitly disabled, or re-enable it with cross-platform compiler and unload policy.

### 6. Gocator Runtime Path Is Platform-Specific
- Cause: parent `CMakeLists.txt` sets `BUILD_RPATH` directly to `modules/Gocator/GoPxL-SDK/lib`.
- Effect: macOS/Linux local builds can find SDK libraries, but Windows runtime deployment is not represented.
- Risk: cross-platform compatibility is incomplete despite project policy.
- Decision needed: add platform-specific runtime handling before claiming Windows build readiness.

### 7. Submodule Language Was Ambiguous
- Cause: docs alternated between "independent repos" and "submodules" without explaining both are true.
- Effect: contributors may either ignore submodule pointer updates or accidentally treat module changes as parent-owned.
- Resolution: docs now state that modules are git submodules for checkout and independent repos for ownership/history.

## Recommended Next Steps
1. Decide the `GraphicsEngineSink` hidden-viewer policy before changing render/backpressure behavior.
2. Decide clear-display semantics for empty static image sessions.
3. Confirm the control-first `DeviceWindow` layout.
4. Gate processing UI by frame domain until Scene3D processing exists.
5. Keep dynamic compilation disabled until cross-platform activation is defined.
