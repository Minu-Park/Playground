# Playground Agent Guide

## Core
- Playground = module creation/removal/composition/test workspace.
- `modules/*` are git submodules with independent git histories.
- Parent repo owns only orchestration source and docs.
- Korean chat, English docs. Do not translate unless asked.
- Concise, word-first replies.
- Cause -> effect.
- Ask before unclear boundary or performance-risk changes.
- Cross-platform compatibility: design code and assets for Windows, Linux, and macOS.

## Layout
- `modules/Camera`: Basler camera module, submodule, separate git history.
- `modules/GraphicsEngine`: reusable visualization library, submodule, separate git history.
- `modules/Gocator`: LMI Gocator module, submodule, separate git history.
- `modules/Resources`: shared Qt qrc/assets/QSS module, submodule, separate git history.
- `src`: Playground app source.
- `CMakeLists.txt`: Playground app CMake entry.
- `docs`: parent project docs. Excludes this file.
- `build`: ignored parent build root.

## Work Loop
- Start: read this file, then task-relevant docs.
- At work start and before publishing, fetch the parent repo and every `modules/*` repo, then compare each checked-out `HEAD` with its fetched upstream.
- Keep every submodule at its current fetched upstream tip. If upstream advances, update the submodule pointer and revalidate the parent integration before publishing.
- Check parent git and every module git separately.
- Keep module changes inside that module repo.
- Keep Playground integration code in `src`.
- Keep style/assets in `modules/Resources`; initialize styling via `Resources::installResources` in the host app only.
- Put parent build directories under `build/`.
- Implement features in small, incremental, and immediately testable stages, verifying each checkpoint manually.
- Ensure cross-platform validation: avoid hardcoded OS-specific paths, fonts, or APIs without guardrails.
- Update docs every structural turn.
- Verify with `git diff --check` and the smallest viable configure/build.
- Never publish a parent commit that pins an unpublished or knowingly stale module commit.

## Design Rules
- Read `docs/DESIGN_GUIDE.md` before UI, QSS, chrome, icon, theme, statusbar, or presentation changes.
- Do not add `setStyleSheet()` outside `modules/Resources/Resources.cpp` without asking first.
- Do not hardcode shared UI colors, margins, padding, radius, font weights, or icon variants in `src`, `Camera`, `Gocator`, or `GraphicsEngine`.
- Widgets expose `objectName` and semantic dynamic properties such as `status`, `messageState`, or `state`; Resources owns the visual result.
- Modules must remain usable without Resources installed. Missing theme/icons may look plain, but core controls must still work.
- Module code must not call `Resources::installResources`; only host apps install the theme.
- Custom `QPainter` visuals may keep local fallback colors only when QSS cannot style them; document the exception in `docs/STRUCTURAL_REVIEW.md`.

## Docs
Use this routing before opening or creating docs:

| Purpose | Read first | Update when |
| --- | --- | --- |
| Build, validation, git, publishing, work loop | `docs/DEVELOPMENT_GUIDE.md` | Commands, checks, priority sequence, or publish flow changes. |
| Repository layout, module ownership, integration flow | `docs/STRUCTURE.md` | Files move, ownership changes, module boundaries change, or app topology changes. |
| Session lifecycle, docks, MDI, controller authority | `docs/SESSION_ARCHITECTURE.md` | `MainWindow`, `DeviceSession`, docks, controllers, or display routing changes. |
| UI design, QSS, theme, chrome, icons, status/message styling | `docs/DESIGN_GUIDE.md` | Any presentation rule, style selector, dynamic property, or Resources UI contract changes. |
| Known risks, boundary debt, deferred decisions | `docs/STRUCTURAL_REVIEW.md` | A concern is real but not fixed now, or a boundary/performance decision is deferred. |
| Basler Stereo 3D hardware contract | `docs/STEREO_3D_CAMERA_INTEGRATION.md` | Stereo mini/ace payload, calibration, hardware validation, or 3D profile behavior changes. |

Task routing:
- UI change: read `DESIGN_GUIDE.md` and the relevant architecture doc only.
- Architecture change: read `STRUCTURE.md`, then `SESSION_ARCHITECTURE.md` if session/UI lifecycle is touched.
- Device/runtime change: read `STRUCTURE.md`, then the touched module docs; read `STEREO_3D_CAMERA_INTEGRATION.md` only for Stereo 3D work.
- Build/git/upload change: read `DEVELOPMENT_GUIDE.md`; do not scan UI docs unless the change touches UI.
- Risk review: read `STRUCTURAL_REVIEW.md`; update it instead of creating a new risk document.

## Doc Policy
- Do not spread the same rule across multiple docs.
- Current facts go in `STRUCTURE.md`.
- Operating commands go in `DEVELOPMENT_GUIDE.md`.
- UI design rules go in `DESIGN_GUIDE.md`.
- Unsettled risks go in `STRUCTURAL_REVIEW.md`.
- Hardware-specific contracts stay in their hardware doc.
- Prefer updating the routed existing doc when it can hold the information clearly.
- Create a new doc only after user approval when the content is a new long-lived domain contract, a large standalone integration guide, or would make the routed doc noisy.
- When a new doc is approved, link it from this `Docs` section in the same change.
- Prefer editing or deleting stale text over adding another status/history document.
- Keep docs short and normative. Put completed history in git commits, not docs.
- If two docs conflict, update the source-of-truth doc above and remove or replace the stale statement.
