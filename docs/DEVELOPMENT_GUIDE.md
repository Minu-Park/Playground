# Playground Development Guide

## Read Order
1. `AGENTS.md`
2. `docs/STRUCTURE.md`
3. Task-relevant module docs

## Commands
```bash
git status --short --branch
git -C modules/Camera status --short --branch
git -C modules/GraphicsEngine status --short --branch
git -C modules/Gocator status --short --branch
git -C modules/Resources status --short --branch
cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
git diff --check
```

## Current Priority
- Keep a minimal Qt host app.
- Central widget: `GraphicsEngine`.
- Right docks: `QCameraWidget`, `QGocatorWidget`.
- Wire Camera 2D/3D and Gocator GDP callbacks into `GraphicsEngine`.
- Keep SDK payload adapters compiled by the host app, not the base GraphicsEngine library.
- Keep shared QSS/icons in `modules/Resources`; initialize them by calling `Resources::installResources(app)` in the host app.

## Rules
- Parent code belongs in `src`.
- Module code belongs in `modules/<Name>`.
- Parent build directories stay under `build/`.
- Check each repo independently before final reporting.
- Keep callback UI writes queued to the GUI thread.
- Keep style changes out of device/engine repos unless they are widget object-name/API changes.
- Ask before changing module ownership, callback admission policy, or render/backpressure behavior.
