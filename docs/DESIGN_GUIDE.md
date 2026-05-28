# Playground Design Guide

## Purpose
- Keep Playground visually consistent without making modules depend on the Playground host.
- Resources provides the installed theme; modules keep plain Qt fallback behavior.
- UI code exposes meaning. QSS owns appearance.

## Ownership
| Area | Owner | Rule |
| --- | --- | --- |
| App theme QSS | `modules/Resources/theme/qss` | All shared colors, spacing, borders, radius, font weight, status bubbles, and chrome selectors live here. |
| Chrome widgets | `modules/Resources/Chrome` | Reusable title bars, dock title bars, and MDI frame presentation only. No session lifecycle or device state. |
| Host composition | `src` | Installs Resources once, wires windows/docks/sessions, and may paint host-only workspace background. |
| Device widgets | `modules/Camera`, `modules/Gocator` | Provide controls, object names, and semantic properties. No shared palette hardcoding. |
| Visualization widgets | `modules/GraphicsEngine` | Provide object names for styleable widgets. Custom painter fallback colors remain local until a theme-token boundary exists. |

## Mandatory Rules
- `Resources::installResources(app)` is called by host apps only.
- Modules must compile and operate without calling Resources APIs.
- Do not add local `setStyleSheet()` outside `modules/Resources/Resources.cpp`.
- Do not add shared UI hex colors, border radius, padding, margin, or font-weight values outside Resources QSS.
- Use stable `objectName` selectors for styleable widgets.
- Use dynamic properties for state:
  - `status`: device status labels such as `Idle`, `Disconnected`, `Connected`, `Live`.
  - `messageState`: device statusbar message labels such as `normal`, `error`.
  - `state`: host status text such as runtime path warnings.
- After changing a dynamic property, repolish the widget when immediate visual refresh is needed.
- Enforce size limits and layout safety:
  - Visual panel constructors must define explicit `setMinimumSize` parameters (e.g., `300x350` for `QCameraWidget`, `380x480` for `QProcessingWidget`) to prevent display compression.
  - `DeviceSession` must dynamically compute minimum bounds in `minimumSizeHint()` depending on active dock visibility.
  - `MdiSubWindowContainer` monitors layout changes via event filters to dynamically update resize limits, preventing hardcoded limits.

## QSS Layout
| File | Scope |
| --- | --- |
| `00_base.qss` | Common Qt widget defaults and low-level shared selectors. |
| `10_graphics_engine.qss` | GraphicsEngine toolbar/statusbar/image/plot helper selectors. |
| `20_statusbar.qss` | Shared statusbar and bubble sizing rules. |
| `30_camera_gocator.qss` | Camera/Gocator control widgets, feature trees, device status/message labels. |
| `40_chrome.qss` | Top-level titlebar, session titlebar, dock titlebar, MDI frame controls. |
| `50_static_image.qss` | Test Image controls and neutral file selection. |
| `60_processing.qss` | Processing dock, editor shell, runtime-path dialog, compile controls. |

Keep selector order stable. Move common selectors earlier and object-specific selectors later.

## Required Pattern
```cpp
label->setObjectName(QStringLiteral("CameraMessageLabel"));
label->setProperty("messageState", "error");
label->style()->unpolish(label);
label->style()->polish(label);
```

```qss
QLabel#CameraMessageLabel[messageState="error"] {
    color: #c62828;
    font-weight: 700;
}
```

## Standalone Fallback
- A module can show plain controls when Resources is not installed.
- Empty icons from `:/Resources/Icons/*` must not break control logic.
- Buttons should remain enabled/disabled by behavior, not by the presence of a theme asset.
- Do not move device logic, render logic, callbacks, or session ownership into Resources to make styling easier.

## QComboBox Inline Dropdown
- QComboBox dropdowns use a GTK-style in-place expansion: the list appears flush below the combo as a single connected rounded rectangle, not as a detached popup.
- `PopupStyleFilter` in `Resources.cpp` repositions the popup container directly below the combo, removes the native frame, and strips the container's QFrame border so the inner `QAbstractItemView` draws the visible border.
- QSS `QComboBox:on` removes the bottom border and bottom radius; `QComboBox QAbstractItemView` draws left/right/bottom borders with bottom radius only. Together they form one seamless box.
- When screen space below is insufficient, the popup appears above the combo instead.

## Frameless Window Interaction
- **DPI-aware Logo Scaling**: Raw logos must be scaled by multiplying the target logical size by `devicePixelRatio()` in C++, then setting `setDevicePixelRatio()` on the scaled pixmap before insertion, preventing upsizing blurriness.
- **DPI-aware Stateful Button Icons**: Custom button icons (such as minimize, maximize, and close buttons) require smooth scaling. Direct QSS `image` paths scale poorly without anti-aliasing. Implement stateful swaps via C++ `eventFilter` on `QEvent::Enter` and `QEvent::Leave` to bypass `QStyleSheetStyle`'s QIcon::Active rendering limitations.
- **Native Window Operations**: For smooth dragging and resizing of frameless windows without jitter or titlebar layout collapse, delegate window dragging to `QWindow::startSystemMove()` and window resizing to `QWindow::startSystemResize()`. If native move is refused during first activation/exposure, fall back to manual dragging for that press.
- **Frameless Analysis Dialogs**: Analysis dialogs (Plot View, Line Profile, Statistics) inside the `GraphicsEngine` module are declared as frameless (`Qt::FramelessWindowHint`) and translucent (`Qt::WA_TranslucentBackground`). They integrate `AnalysisDialogTitleBar` at the top of their layouts for native-like window dragging and close operations.


## Exceptions
- Custom `QPainter` surfaces cannot be fully styled by QSS. Keep local fallback colors until a dedicated theme-token API is designed.
- Any new exception must be listed in `docs/STRUCTURAL_REVIEW.md` with cause, effect, and required direction.

## Review Checklist
- `rg -n "setStyleSheet\\(" src modules/Camera modules/Gocator modules/GraphicsEngine modules/Resources --glob '!modules/Resources/theme/qss/*.qss' --glob '!modules/Resources/Style.qss'`
- `rg -n "QColor\\(|#[0-9A-Fa-f]{6}|font-weight|border-radius|padding:|margin:" src modules/Camera modules/Gocator modules/GraphicsEngine --glob '!modules/Gocator/GoPxL-SDK/**'`
- Confirm modules do not call `Resources::installResources`.
- Run `git diff --check` in parent and touched modules.
- Build the smallest affected target, then `Playground` for integration changes.
