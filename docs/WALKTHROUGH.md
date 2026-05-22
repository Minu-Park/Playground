# Walkthrough (MDI Layout & OpenCV C++ Dynamic Compilation)

모든 6가지 마일스톤에 대한 구현이 성공적으로 완료되었으며, 빌드가 검증되었습니다.

## 1. 구현 완료된 기능
- **LMI Gocator 3D Callback 연동**: `GocatorDataSetScene3DAdapter`를 사용해 3D 포인트 클라우드 렌더링에 필요한 `GraphicsScene3D` 데이터를 `GraphicsEngine`에 스레드 안전하게 연결(`QMetaObject::invokeMethod` 및 `Qt::QueuedConnection` 적용).
- **CMake OpenCV 조건부 종속성**: OpenCV가 로컬 환경에 있을 때 `HAS_OPENCV`를 정의하고 `OPENCV_INCLUDE_DIR` 및 `OPENCV_LIB_DIR`를 주입.
- **Pylon-to-OpenCV 변환 헬퍼**: Pylon 이미지 버퍼(`const void*`)를 `cv::Mat`으로 안전하게 변환하는 헬퍼 및 `cv::Mat`을 `QImage`로 변환해주는 헬퍼 작성.
- **실시간 C++ 동적 컴파일 패널 (`QProcessingWidget`)**:
  - `QTextEdit`을 사용한 OpenCV C++ 필터 스크립트 작성 환경 제공.
  - 파라미터 조절용 실시간 슬라이더 구현.
  - `QProcess`를 이용해 백그라운드에서 `clang++ -shared -fPIC` 빌드를 수행하여 `libdynamic_filter.dylib` 생성.
  - `QLibrary`를 통해 생성된 dylib에서 `processImage` 필터 심볼을 런타임에 동적으로 로드 및 교체.
- **실시간 이미지 처리 파이프라인 연동**: 카메라 그랩 루프에 동적으로 로드된 OpenCV 필터 함수 포인터를 `std::mutex` 임계 영역 동기화를 통해 가로채어 실시간 처리 및 화면 갱신 수행.

## 2. 변경 및 신규 추가된 파일 목록

| 파일 경로 | 수정 상태 | 주요 역할 |
| :--- | :--- | :--- |
| [CMakeLists.txt](file:///Users/minwoo/Documents/Projects/Playground/CMakeLists.txt) | **MODIFY** | OpenCV 패키지 조건부 검색, OPENCV_INCLUDE_DIR / OPENCV_LIB_DIR 매크로 정의, `QProcessingWidget.cpp` 소스 빌드 대상에 추가. |
| [src/DeviceWindow.h](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.h) | **MODIFY** | `QProcessingWidget` 및 OpenCV 관련 필터링 전역 멤버 및 뮤텍스 선언. |
| [src/DeviceWindow.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.cpp) | **MODIFY** | Gocator 그랩 콜백 등록 및 3D 그래픽스 엔진 연동. Pylon/OpenCV 이미지 형식 상호 변환 헬퍼 추가. 카메라 그랩 콜백 내부에서 동적 C++ OpenCV 필터 호출 파이프라인 추가. |
| [src/QProcessingWidget.h](file:///Users/minwoo/Documents/Projects/Playground/src/QProcessingWidget.h) | **NEW** | 실시간 런타임 컴파일 에디터 헤더 파일. |
| [src/QProcessingWidget.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/QProcessingWidget.cpp) | **NEW** | `QProcess` 기반 `clang++` 컴파일러 구동 및 `QLibrary` 동적 로딩 소스 파일. |

## 3. 검증 결과
- `cmake -S . -B build/cmake-build-debug`를 통한 CMake 빌드 설정 완벽 검증.
- `cmake --build build/cmake-build-debug --target Playground -j 8`로 `Playground` 타겟의 에러 없는 컴파일 및 링킹 빌드 확인.
- 로컬의 OpenCV 라이브러리 및 헤더 경로가 빌드 타겟과 런타임 컴파일 매크로로 정상 전달됨 확인.

## 4. 트러블슈팅 (OpenCV 라이브러리 경로 오류 수정)
- **증상**: 런타임 C++ 에디터에서 `Compile & Apply` 실행 시, `ld: library 'opencv_core' not found` 링커 에러가 출력되면서 빌드 실패.
- **원인**: `CMakeLists.txt`에서 런타임 컴파일 매크로 공급을 위해 `OPENCV_LIB_DIR` 경로를 계산하는 과정 중, 이미 `/opt/opencv/lib` 경로를 얻었음에도 링커 플래그에 `/lib`을 중복해서 덧붙여 `-L/opt/opencv/lib/lib` 형태가 됨.
- **해결**: 중복되었던 `/lib` 추가를 삭제하여 링커 플래그가 정확히 `-L/opt/opencv/lib`로 빌드되게끔 [CMakeLists.txt](file:///Users/minwoo/Documents/Projects/Playground/CMakeLists.txt#L54-L64)를 수정 및 정상 컴파일 통과 완료.

---

# UI 개편 및 디버깅용 전역 롤링 로깅 시스템 (기기별 하단 독 제거 및 MDI 배경 개선 포함)

## 1. 구현 완료된 기능
- **OpenCV 일시 비활성화**:
  - `CMakeLists.txt` 내 OpenCV 관련 빌드 설정 및 `HAS_OPENCV` 컴파일 매크로를 주석 처리하여, 향후 재사용을 기약하면서 일시 비활성화하였습니다.
- **UI Menu bar 개편 & 아이콘 비활성화**:
  - 기존의 `QToolBar`를 완전히 제거하고, 상단 Menu bar (`Device`, `Window`)를 통해 Basler Camera 및 LMI Gocator 생성 기능을 수행하도록 변경했습니다.
  - 사용자의 피드백을 반영하여 Device 메뉴 액션들에서 생성자 `QIcon` 인자를 제거하고, **아이콘 없이 텍스트로만 심플하게 표시**되도록 변경했습니다.
- **전역 로깅 시스템 (`LogManager`) 및 표준 스트림 리다이렉터 구현**:
  - `qInstallMessageHandler`를 등록하여 전체 프로젝트 내의 `qDebug()`, `qWarning()`, `qCritical()`, `qFatal()` 로그를 일괄 수집합니다.
  - **표준 C++ 스트림 가로채기 (`StdStreamRedirector`)**: `modules/Camera` 등 외부 모듈에서 `std::cout` 및 `std::cerr`를 직접 호출하여 발생하는 로그들(예: `[Camera System] ...`)을 가로채어 `LogManager`의 로깅 파이프라인으로 안전하게 라우팅합니다. 무한 재귀 루프를 방지하기 위해 콘솔 출력 시 원본 스트림 버퍼를 우회하도록 설계되었습니다.
- **500줄 FIFO 롤링 파일 로그 (`lastlog.log`)**:
  - 메모리 버퍼 큐 (`QQueue<QString>`)에 로그를 최대 500줄 보관하며, 이를 초과할 경우 선입선출(FIFO) 방식으로 오래된 로그를 지웁니다.
  - 실행 파일과 동일 디렉토리(`QCoreApplication::applicationDirPath() + "/lastlog.log"`)에 덮어쓰기 방식으로 저장합니다.
- **화이트 디자인 테마 로그 뷰어 Dock**:
  - 애플리케이션의 전체 화이트 QSS 테마 철학과 자연스럽게 조화를 이루도록 로그 창 디자인을 리펙토링했습니다 (배경 `#ffffff`, 텍스트 `#16202b`, 보더 `#d9e1ea` 스타일링 적용).
  - 로그 창 출력 시 로그 레벨별로 구분된 세련된 HSL 컬러 테일러드 마크업(HTML Rich Text)을 지원하여 가독성을 극대화했습니다.
  - 백그라운드 스레드 안정성을 보장하기 위해 `Qt::QueuedConnection`을 사용해 `LogManager::logAdded` 시그널과 `MainWindow::appendLog` 슬롯을 동기화 연결했습니다.
- **DeviceWindow 내 OpenCV/PCL 하단 독 제거**:
  - `DeviceWindow` 내부에서 OpenCV/PCL 실시간 코드 편집 패널 역할을 하던 하단 독(`_processingDock`)과 `QProcessingWidget`을 제거하여 센서 제어와 시각화 화면의 비중을 높였습니다.
- **QMdiArea 배경 디자인 개선 (깔맞춤)**:
  - MDI 영역 바탕화면을 중립 그레이 `#eeeeee`로 칠하고, 중앙에 `:/Resources/BASLER_Logo.png`를 50% 크기로 그리는 host-side `BrandedMdiArea`를 적용했습니다.
  - 배경 painting은 `src/MainWindow.cpp`에 남기고, 로고 asset은 `modules/Resources`의 QRC 경로를 참조합니다.

## 2. 변경 및 신규 추가된 파일 목록

| 파일 경로 | 수정 상태 | 주요 역할 |
| :--- | :--- | :--- |
| [CMakeLists.txt](file:///Users/minwoo/Documents/Projects/Playground/CMakeLists.txt) | **MODIFY** | OpenCV 빌드 매크로 비활성화 주석 처리 및 `LogManager.cpp` 빌드 소스 추가. |
| [src/main.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/main.cpp) | **MODIFY** | `QApplication` 생성 직후 `LogManager::instance()->init()` 호출해 전역 핸들러 및 리다이렉터 등록. |
| [src/LogManager.h](file:///Users/minwoo/Documents/Projects/Playground/src/LogManager.h) | **MODIFY** | `LogManager` 싱글톤 및 `StdStreamRedirector` 관리를 위한 헤더 및 멤버 선언 수정. |
| [src/LogManager.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/LogManager.cpp) | **MODIFY** | `StdStreamRedirector` 클래스 구현, `std::cout`/`std::cerr` 가로채기, HTML 가독성 태그 입힌 GUI 콘솔 송신 및 무한 루프 예방용 로직 적용. |
| [src/MainWindow.h](file:///Users/minwoo/Documents/Projects/Playground/src/MainWindow.h) | **MODIFY** | 로그 Dock 연동 멤버 및 `appendLog` GUI 갱신 슬롯 선언. |
| [src/MainWindow.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/MainWindow.cpp) | **MODIFY** | 하단 QDockWidget 로그창 추가, 화이트 테마 디자인 스타일시트 적용, `Qt::QueuedConnection` 연동 및 QToolBar 제거. 메뉴 액션에서 QIcon 제거. |
| [src/DeviceWindow.h](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.h) | **MODIFY** | `_processingDock` 및 `_processingWidget` 관련 선언과 forward declaration 제거. |
| [src/DeviceWindow.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.cpp) | **MODIFY** | OpenCV/PCL 하단 독 및 `QProcessingWidget` 생성/시그널 연동 로직 제거. |
| [modules/Resources/Style.qss](file:///Users/minwoo/Documents/Projects/Playground/modules/Resources/Style.qss) | **MODIFY** | 화이트 테마에 최적화된 모던하고 세련된 `QMdiArea` 배경 스타일 추가. |

## 3. 검증 결과
- **CMake & Build**: `cmake -B build -S .` 및 `cmake --build build` 정상 통과 완료.
- **동작 검증**:
  - `-platform offscreen` 플래그로 CLI 환경에서 Playground 애플리케이션을 구동하여 윈도우 생성 없이 로그 파이프라인 검증 성공.
  - `std::cout` 및 `std::cerr`로 출력된 임시 디바이스 동작 로그들이 리다이렉션되어 `build/lastlog.log`에 `[INFO]`, `[WARN]` 레벨로 가공되고 타임스탬프와 함께 완벽히 기록됨을 검증 완료.

## 4. 로그창 토글 기능 및 스마트 자동 스크롤 추가 개선 (2026-05-22 추가)
- **Window 메뉴에 시스템 로그 토글 추가**:
  - `QDockWidget::toggleViewAction()` 기능을 활용하여 Window 메뉴 하단에 "System Logs" 항목을 등록했습니다.
  - 사용자가 실수로 로그 창을 닫아도 메뉴바를 통해 직관적으로 띄우고 숨길 수 있으며, 메뉴의 체크 상태와 실제 Dock의 가시성이 양방향으로 자동 연동됩니다.
- **사용자 경험을 배려한 스마트 자동 스크롤(Smart Autoscroll)**:
  - 로그가 추가되기 전 뷰포트 상태를 분석하여, 수직 스크롤바가 최하단 부근(허용 오차 10px 이내)에 위치했을 때만 새 로그 추가 후 최하단으로 자동 스크롤시킵니다.
  - 사용자가 이전 기록 분석을 위해 스크롤을 위로 올려두었을 경우에는 뷰포트 위치를 그대로 보존하여 읽기 흐름을 방해하지 않습니다.
  - 사용자가 다시 스크롤바를 맨 아래로 가져다 놓으면, 이후 들어오는 새 로그에 대해 자동으로 뷰포트 추적이 재개됩니다.

## 5. UI 깜빡임 현상 및 폰트 앨리어스 경고 트러블슈팅 (2026-05-22 추가)
- **카메라/고케이터 윈도우 생성 시 전체 윈도우 번쩍임(Reparenting Flicker) 해결**:
  - 기존에는 `DeviceWindow`를 생성할 때 부모(`parent`) 인자로 `MainWindow(this)`를 전달했습니다. 이로 인해 생성 직후 부모가 탑레벨 상태에서 `QMdiArea::addSubWindow`로 넘어가면서 MDI 자식 윈도우로 Reparenting되는 오버헤드가 발생, 윈도우가 통째로 깜빡였습니다.
  - 생성자 파라미터 `parent`에 `nullptr`을 명시하여 탑레벨 리페어런팅 드로잉 부담을 제거함으로써, MDI 윈도우가 깜빡임 없이 부드럽게 생성되도록 수정 완료했습니다.
- **멀티 플랫폼 폰트 폴백 체인 구축 및 macOS 지연 경고 해결**:
  - `Sans Serif` 및 `Consolas` 폰트 부재로 구동 시 macOS에서 발생하던 폰트 앨리어스 조회 지연 경고(`Populating font family aliases took 51 ms`)를 해결했습니다.
  - `main.cpp`에서 macOS 기본 시스템 폰트(`.AppleSystemUIFont`)를 기본값으로 지정하여 조회 부하를 우회했습니다.
  - `MainWindow.cpp` 스타일시트에 Windows(Consolas), Linux(DejaVu Sans Mono, Liberation Mono), macOS(Menlo, Monaco)의 대표적인 코딩용 고정폭 폰트들을 모두 포함시킨 멀티 플랫폼 폴백 리스트(`'Menlo', 'Monaco', 'Consolas', 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New', monospace`)를 적용해 교차 플랫폼 환경에서 폰트 경고 및 성능 저하 없이 미려하게 표현되도록 개선했습니다.

---

# Camera & Gocator UI Status Display and Parameter Notification (StatusBar Integration) (Added 2026-05-22)

## 1. Key Implementation Features
- **Operation & Status Feedback**:
  - Displays a confirmation message on the `QStatusBar` when GenApi parameters are successfully modified or when device discovery / connection operations complete.
  - Displays error details for 5 seconds on Camera parameter/command failures. Gocator discovery/connection status text uses the normal message style unless a dedicated warning path is added later.
- **Error Message Visualization**:
  - Camera error messages use a bold red transient style. Gocator generic messages such as `Connection Failed` and `No devices found` stay in the default message font/color.
- **Clean and Modern Status Indicators (Bubble Style) - Left Aligned**:
  - Uses one dynamic status bubble per device widget: `#CameraStatusLabel` / `#GocatorStatusLabel`.
  - The bubble text switches between `Disconnected`, `Connected`, and `Live`.
  - Bubble backgrounds stay white; state is communicated by text color (`red`, `green`, `blue`) and a thin neutral border.
  - **QSS Isolation & Dynamic Property Styling**:
    - Styling is offloaded to `modules/Resources/Style.qss` using dynamic properties (`status="connected"`, `status="grabbing"`, etc.).
    - Calls `QStyle::unpolish` and `QStyle::polish` to trigger stylesheet re-evaluation dynamically upon status changes, keeping UI repaint responsive and performant.
- **Stability and Crash Fixes**:
  - **Unique Connection Assertion Fix**: Moved the timer timeout connection logic to the constructor instead of multiple connect calls with `Qt::UniqueConnection` inside `showStatusMessage()`, solving the runtime ASSERT failure.
  - **Destructor Safety**: Ensures device `stop()` and `close()` are called prior to callback deregistration in `~DeviceWindow` to prevent multi-threaded callbacks executing on destroyed objects.
  - **MDI Shutdown Ordering**: `MainWindow` deletes MDI subwindows before `CameraSystem` destruction so `DeviceWindow` can deregister callbacks and remove cameras while the owning system is still valid.

## 2. Modified Files List

| File Path | Status | Key Role |
| :--- | :--- | :--- |
| [QCameraWidget.h](file:///Users/minwoo/Documents/Projects/Playground/modules/Camera/C++/Utility/Qt/QCameraWidget.h) | **MODIFY** | Declares `QLabel` pointers for status indicators and the message timer. |
| [QCameraWidget.cpp](file:///Users/minwoo/Documents/Projects/Playground/modules/Camera/C++/Utility/Qt/QCameraWidget.cpp) | **MODIFY** | Instantiates status labels, integrates them to the left side of status bar, and implements connection/grab state updating logic with dynamic properties. |
| [QGocatorWidget.h](file:///Users/minwoo/Documents/Projects/Playground/modules/Gocator/C++/Utility/Qt/QGocatorWidget.h) | **MODIFY** | Declares `QLabel` pointers for status indicators and the message timer for Gocator. |
| [QGocatorWidget.cpp](file:///Users/minwoo/Documents/Projects/Playground/modules/Gocator/C++/Utility/Qt/QGocatorWidget.cpp) | **MODIFY** | Integrates connection and grab status bubbles to the left side of the status bar, maps `setStatus` text updates to `showStatusMessage`, and implements dynamic property updates. |
| [Style.qss](file:///Users/minwoo/Documents/Projects/Playground/modules/Resources/Style.qss) | **MODIFY** | Mappings for both `Camera` and `Gocator` status label bubble styles. |
| [DeviceWindow.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.cpp) | **MODIFY** | Improves destructor sequence stability (`stop` prior to callback deregistration) to prevent crash on exit. |

## 3. Verification Results
- **CMake & Build**: Successfully compiled with `cmake --build build/cmake-build-debug --target Playground`.
- **Git Formatting**: Passed `git diff --check` with no whitespace warnings.
- **Status Jitter Prevention**: Resolved layout shifts during parameter modification messages by setting the message label size policy to `Ignored` (allowing expanding behavior only in layout allocation, without affecting the widget's own sizeHint feedback) and specifying a minimum width for connection and grabbing status bubbles in `Style.qss`.
- **Unified Status Bubble**: Consolidated separate connection and grabbing status labels into a single `#CameraStatusLabel` / `#GocatorStatusLabel` bubble to simplify the status bar layout, displaying `Disconnected`, `Connected`, and `Live` dynamically.
- **Softer Bubble Aesthetics**: Updated `Style.qss` to use white backgrounds (`#ffffff`) for all states, using only text colors and thin colored borders for status distinction (`Disconnected` in red, `Connected` in green, `Live` in blue). Removed `min-width` to allow bubbles to resize dynamically based on their text content.

---

# 2D Image Testing Simulation Structure (Add Image) (Added 2026-05-22)

## 1. Key Implementation Features
- **Offline Simulation Support**:
  - Implemented `StaticImageImagingController` (inheriting from `AbstractImagingController`) to allow processing offline static images from disk through the `ProcessingPipeline` without camera hardware.
- **Playback & Navigation Control**:
  - Added `QStaticImageControlWidget` hosted inside `DeviceWindow`'s control dock when in static image mode.
  - Supports adding images dynamically during runtime, removing images, manual navigation (Prev/Next), automatic loop playback (Play/Pause), and adjusting playback speed (FPS) via a slider.
- **MDI Integration & File Dialog**:
  - Added "Add Test Image Session..." to the main window menu.
  - Opens a file dialog (`QFileDialog`) to select multiple images, spawning a dedicated `DeviceWindow` linked to the static simulation.

## 2. Modified & Added Files List

| File Path | Status | Key Role |
| :--- | :--- | :--- |
| [CMakeLists.txt](file:///Users/minwoo/Documents/Projects/Playground/CMakeLists.txt) | **MODIFY** | Registers static image controller and control widget to build targets. |
| [StaticImageImagingController.h](file:///Users/minwoo/Documents/Projects/Playground/src/StaticImageImagingController.h) / [.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/StaticImageImagingController.cpp) | **NEW** | Simulation controller that wraps image load and pipelines. |
| [QStaticImageControlWidget.h](file:///Users/minwoo/Documents/Projects/Playground/src/QStaticImageControlWidget.h) / [.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/QStaticImageControlWidget.cpp) | **NEW** | Control panel UI with play/pause, prev/next, file lists, and FPS slider. |
| [DeviceWindow.h](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.h) / [.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/DeviceWindow.cpp) | **MODIFY** | Integrates static image initialization and control widget binding. |
| [MainWindow.h](file:///Users/minwoo/Documents/Projects/Playground/src/MainWindow.h) / [.cpp](file:///Users/minwoo/Documents/Projects/Playground/src/MainWindow.cpp) | **MODIFY** | Adds menu trigger to load test image sessions. |
| [STRUCTURE.md](file:///Users/minwoo/Documents/Projects/Playground/docs/STRUCTURE.md) | **MODIFY** | Details architecture and simulation flow changes. |

## 3. Verification Results
- **CMake & Build**: Successfully configured and compiled target `Playground` with no errors or warnings.
- **Git diff check**: No trailing whitespace issues found via `git diff --check`.

