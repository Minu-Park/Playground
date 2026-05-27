# Structural Review

## Scope
- Review date: 2026-05-27.
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
| Medium | OpenCV discovery vs CMake policy lifetime | Parent CMake sets `CMP0146` to `OLD` because the installed OpenCV config still probes CUDA through CMake's removed legacy module. | Configure currently succeeds with a deprecation warning, but a future CMake version may reject the fallback. | Establish a compatible OpenCV/CMake discovery path, then remove the `OLD` policy setting. |
| Medium | Dynamic compile policy vs UI ownership | The unused `create_node` path was removed; the live `process_image` ABI, compiler invocation, scratch files, and `.dylib` output still live inside `QProcessingWidget`. | Enabled OpenCV builds expose one active but macOS-specific extension path that cannot satisfy the cross-platform rule. | Extract platform-aware compile/load policy from the widget before extending the node contract. |
| Medium | OpenGL composition seed vs startup cost | `QVTKOpenGLNativeWidget` derives from `QOpenGLWidget`, and Qt 6.4+ can recreate a visible top-level window when its first OpenGL child is inserted. First-session-only reproduction on macOS and Linux confirmed this path; `MainWindow` now creates a transparent one-pixel `QOpenGLWidget` before first show. | The seed moves surface composition setup before visible UI and resolved the first-session refresh in macOS validation, but adds one persistent GL context on every platform. | Repeat first-session verification on Linux and retain the seed only while startup/background appearance remains acceptable. |
| Low | Runtime dependency policy vs cross-platform claim | Gocator/pylon runtime discovery and deployment handling are platform-specific. | Windows/macOS support cannot be claimed from Linux integration builds alone. | Maintain per-platform runtime verification before distribution claims. |

## Repository Rule
- Parent commits contain host code, docs, and submodule pointers.
- Camera and GraphicsEngine implementation changes are committed in their own module repositories first.
