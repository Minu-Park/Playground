# Structural Review

## Scope
- Review date: 2026-05-28.
- Scope: parent app, modified `Gocator` module surface.
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
| Medium | OpenCV discovery vs CMake policy lifetime | Parent CMake still sets `CMP0146` to `OLD` when probing OpenCV as a default runtime provider because the installed OpenCV config probes CUDA through CMake's removed legacy module. | The host now builds without OpenCV, but configure-time OpenCV discovery can still emit a deprecation warning when used for defaults. | Keep OpenCV optional for the host build; remove or isolate the `OLD` policy once runtime/app path providers are sufficient. |
| Medium | Dynamic compile policy vs plugin ABI | `DynamicProcessingCompiler` now owns generated source, compiler arguments, and output paths for `process_image` ABI v1, and `RuntimePathsDialog` exposes OpenCV path overrides. The broader multi-node/plugin ABI is not defined. | OpenCV processing can grow beyond a UI-owned compile command, but ABI expansion still needs an explicit contract. | Keep `process_image` as ABI v1 until a broader plugin ABI is designed. |
| Medium | OpenGL composition seed vs startup cost | `QVTKOpenGLNativeWidget` derives from `QOpenGLWidget`, and Qt 6.4+ can recreate a visible top-level window when its first OpenGL child is inserted. First-session-only reproduction on macOS and Linux confirmed this path; `MainWindow` now creates a transparent one-pixel `QOpenGLWidget` before first show. | The seed moves surface composition setup before visible UI and resolved the first-session refresh in macOS validation, but adds one persistent GL context on every platform. | Repeat first-session verification on Linux and retain the seed only while startup/background appearance remains acceptable. |
| Low | Runtime dependency policy vs cross-platform claim | Gocator, pylon, OpenCV, and similar runtime dependencies can be installed at different versions and paths per machine. System discovery alone cannot cover every deployment. | Windows/macOS/Linux behavior can diverge, and missing SDK paths can fail late or silently without an app-level path policy. | Prefer system-installed discovery, but add app/user/deploy-time path injection. Bundle only components whose redistribution terms are verified, and report missing paths explicitly. |

## Repository Rule
- Parent commits contain host code, docs, and submodule pointers.
- Camera, GraphicsEngine, Gocator, and Resources implementation changes are committed in their own module repositories first.

## Decision Queue
- 3D point-cloud materialization/data contract: keep the source-neutral `GraphicsScene3D` contract. Review zero-copy or move-only improvements case by case only when a measured copy/lifetime cost is identified.
- Dynamic OpenCV plugin contract: `process_image` remains ABI v1. Multi-node/plugin ABI and settings UI are still separate design work.

## Implementation Checklist
- [x] Keep 2D acquisition on strict frame-by-frame backpressure.
- [x] Keep hidden viewers as non-consuming render targets.
- [x] Add explicit clear-display semantics for empty static image sessions.
- [x] Guard Camera callback `ready()` on exception paths.
- [x] Remove legacy Camera `ProcessFunc` filter path.
- [x] Make processing node enable state thread-safe.
- [x] Move OpenCV compile/source/link planning out of `QProcessingWidget`.
- [x] Allow Playground host builds without OpenCV.
- [x] Add OpenCV runtime path resolution through settings, environment, app-local folders, and CMake defaults.
- [x] Attach OpenCV runtime path UI to the filter script toolbar.
- [ ] Generalize runtime path policy for pylon and GoPxL/Gocator SDKs.
- [ ] Add explicit runtime diagnostics UI/logging for missing pylon, GoPxL, and OpenCV paths.
- [ ] Define `process_image` ABI v1 documentation and compatibility rules.
- [ ] Decide whether multi-node processing requires a new ABI or a graph-level wrapper around ABI v1.
- [ ] Add manual validation notes for macOS/Linux/Windows OpenCV compile/load paths.

## Settled Decisions
- 2D live acquisition keeps strict frame-by-frame backpressure. `Camera::ready()` remains tied to processing completion; latest-frame/drop behavior is not part of the default acquisition path.
- Hidden viewers do not retain or consume new display frames. `GraphicsEngineSink` skips hidden targets so visibility remains a render-cost boundary, not a frame-storage policy.
- Keep the startup OpenGL composition seed. It is a stability guard for first-session `QOpenGLWidget` insertion; do not remove it unless a platform-specific validation proves it unnecessary.
