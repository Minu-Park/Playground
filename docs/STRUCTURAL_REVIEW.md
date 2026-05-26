# Structural Review

## Scope
- Review date: 2026-05-26.
- Scope: parent app, modified `Camera` and `GraphicsEngine` module surfaces.
- Purpose: unclear ownership boundaries only. Current layout belongs in `STRUCTURE.md`; completed history belongs in git.

## Active Boundary Debt

| Priority | Boundary | Cause | Effect | Required Direction |
| --- | --- | --- | --- | --- |
| High | Camera stream configuration vs display profile policy | `Camera::configureStereoMiniStream()` selects components and hardcodes mapping mode `None` while registration is a display/use-case choice. | Raw color and hardware-registered color cannot be selected explicitly; future UI risks writing acquisition internals directly. | Extract selectable pylon stream profiles owned by Camera; host selects a named profile only. |
| High | Pylon frame decoding vs scene assembly | `PylonScene3DAdapter.cpp` contains component lookup, byte/stride safety, RGB conversion, direct XYZ decoding, ace reconstruction, metadata assembly, and Blaze delegation. | New auxiliary channels or formats increase one hot-path file and make product regression isolation difficult. | Split shared pylon buffer decoder, product geometry decoder, and neutral scene assembler after current hardware baseline is verified. |
| High | Scene state vs display policy vs UI synchronization | `GraphicsEngine::applyScene3D()` mutates retained resources, chooses default mode, applies backend data, and updates chrome. | Adding registered color/channel selection risks state and UI regressions in the same function. | Introduce a scene-state transition/result step before backend/chrome work. |
| High | Color image identity vs registration UX | `GraphicsSceneState::image` holds ordinary 2D images and stereo color resources, while registration metadata is stored but not exposed as a selectable view/state. | Users cannot distinguish raw color, registered color, and scalar range presentation reliably. | Define color/range channel-selection UX before adding hardware registration defaults. |
| Medium | Host orchestration vs SDK conversion | `CameraImagingController` includes and invokes a pylon adapter because the module callback exposes raw `CPylonDataContainer`. | Parent host must compile against pylon transformation details to integrate Camera. | Keep as current integration boundary; reconsider converted callback/public adapter API only when another host consumer exists. |
| Medium | Blaze compatibility vs pylon common decoder | Blaze remains a separate adapter while Stereo paths implement new component/stride handling in the facade. | Shared safety fixes can diverge across Basler 3D families. | Consolidate common buffer validation after Blaze regression tests exist. |
| Medium | Processing dock vs frame domain | Processing controls are hosted for sessions although Scene3D node semantics remain undefined. | UI advertises capability beyond implemented processing behavior. | Gate or label Scene3D pass-through before enabling 3D node work. |
| Medium | Empty source vs display clearing | `GraphicsEngineSink::enqueueImage` ignores null images used when the last static image is removed. | Stale display can survive an empty session. | Add explicit clear-display semantics. |
| Medium | Dynamic loader ABI vs compile UI | `DynamicLibraryLoader` expects `create_node`, while `QProcessingWidget` resolves `process_image` and owns a second wrapper/ABI; compile output is `clang++`/`.dylib`-fixed. | Enabled OpenCV builds expose a platform-specific path with two extension contracts. | Decide one ABI and extract platform-aware compiler/load policy from the widget. |
| Low | Runtime dependency policy vs cross-platform claim | Gocator/pylon runtime discovery and deployment handling are platform-specific. | Windows/macOS support cannot be claimed from Linux integration builds alone. | Maintain per-platform runtime verification before distribution claims. |

## Resolved In This Cleanup
- Removed duplicate `GraphicsScene3DRequest::includeColorImage`; `GraphicsScene3DContent::ColorImage` is now the single request signal.
- Moved decoder facts out of the `Camera` class contract into `PylonScene3DProfile`; adapter no longer depends on the acquisition class.
- Removed unused profile capability-looking fields; actual frame components remain authoritative for RGB decoding.
- Deleted completed/redundant host walkthrough and imaging-controller plan documents.
- Removed hardcoded `/opt/opencv` CMake discovery; OpenCV now follows optional package discovery while dynamic ABI/platform policy remains listed debt.

## Repository Rule
- Parent commits contain host code, docs, and submodule pointers.
- Camera and GraphicsEngine implementation changes are committed in their own module repositories first.
