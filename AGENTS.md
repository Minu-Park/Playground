# Playground Agent Guide

## Core
- Playground = module creation/removal/composition/test workspace.
- `modules/*` are independent git repositories.
- Parent repo owns only orchestration source and docs.
- Korean chat, English docs.
- Concise, word-first replies.
- Cause -> effect.
- Ask before unclear boundary or performance-risk changes.
- Cross-platform compatibility: design code and assets for Windows, Linux, and macOS.

## Layout
- `modules/Camera`: Basler camera module, separate git history.
- `modules/GraphicsEngine`: reusable visualization library, separate git history.
- `modules/Gocator`: LMI Gocator module, separate git history.
- `modules/Resources`: shared Qt qrc/assets/QSS module, separate git history.
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
- `docs/DEVELOPMENT_GUIDE.md`: priorities, commands, work checklist.
- `docs/STRUCTURE.md`: current layout, ownership, integration flow.
- `docs/DYNAMIC_SCRIPTING_PROPOSAL.md`: dynamic OpenCV C++ runtime compilation proposal.
