# Playground Agent Guide

## Core
- Playground = module creation/removal/composition/test workspace.
- `modules/*` are git submodules with independent git histories.
- Parent repo owns only orchestration source and docs.
- Korean chat, Korean reports/docs (English only for search-based docs).
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
- Check parent git and touched module git separately.
- Keep module changes inside that module repo.
- Keep Playground integration code in `src`.
- Keep style/assets in `modules/Resources`; initialize styling via `Resources::installResources` in the host app.
- Put parent build directories under `build/`.
- Implement features in small, incremental, and immediately testable stages, verifying each checkpoint manually.
- Ensure cross-platform validation: avoid hardcoded OS-specific paths, fonts, or APIs without guardrails.
- Update docs every structural turn.
- Verify with `git diff --check` and the smallest viable configure/build.

## Docs
- `docs/NEXT_SESSION_HANDOFF.md`: active published baseline and next-session first actions, when present.
- `docs/DEVELOPMENT_GUIDE.md`: priorities, commands, work checklist.
- `docs/STRUCTURE.md`: current layout, ownership, integration flow.
- `docs/SESSION_ARCHITECTURE.md`: class-by-class session authority and UI/lifecycle boundary.
- `docs/DYNAMIC_PROCESSING_STATUS.md`: dynamic OpenCV C++ runtime processing status and unresolved boundary.
- `docs/STRUCTURAL_REVIEW.md`: current structural risks and decisions needed.
