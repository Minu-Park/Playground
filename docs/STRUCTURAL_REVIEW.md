# Structural Review

## Scope
- Review date: 2026-05-24.
- Scope: parent Playground repo plus observed module status.
- Code was changed after this review to introduce the `DeviceSession` authority boundary.
- Items below are cause -> effect findings.

## Git State
- Check parent and touched module repositories separately.
- Parent commits should contain host code, docs, and submodule pointers only.
- Module commits should be made inside the touched module repository before the parent submodule pointer commit.

## Findings

### 1. Viewer Visibility No Longer Belongs To Device Control Panels
- Cause: `GraphicsEngine` is the `DeviceSession` central widget instead of a dock attached to a device control surface.
- Effect: hiding Camera/Gocator/Test Image control panels does not hide the viewer and does not stop display routing.
- Risk: explicit full-session close and explicit acquisition stop actions must remain visually distinct from dock hide actions.
- Decision: keep control panel visibility separate from acquisition lifecycle.

### 2. Empty Image Clears Are Blocked
- Cause: `GraphicsEngineSink::enqueueImage` returns early for null images.
- Effect: `StaticImageImagingController::removeImage` cannot clear the viewer after the last file is removed because it enqueues `QImage()`.
- Risk: stale image content can remain visible after the session becomes empty.
- Decision needed: add an explicit clear-display API, or allow null-image clear semantics in the sink.

### 3. Session Authority Was Renamed
- Cause: `DeviceWindow` implied that a closeable control window owned session truth.
- Effect: the class is now `DeviceSession`, with central `GraphicsEngine` and docked control panels.
- Risk: old docs or future code may reintroduce UI-widget lifetime as hardware lifetime.
- Decision: use `DeviceSession` for session authority and reserve control widgets for UI only.

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

### 8. Status Styling Is Shared
- Cause: Camera and Gocator widgets both expose connection state through status labels.
- Effect: the `Idle`, `Disconnected`, `Connected`, and `Live` palette must stay in `modules/Resources`, while each device widget only sets the dynamic `status` property.
- Risk: hardcoding colors in device widgets would split the UI contract again.
- Decision: keep shared status colors in Resources and source-specific state transitions in each device module.

## Recommended Next Steps
1. Decide clear-display semantics for empty static image sessions.
2. Gate processing UI by frame domain until Scene3D processing exists.
3. Keep dynamic compilation disabled until cross-platform activation is defined.
