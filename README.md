# Playground

> Basler 카메라, LMI Gocator 3D 센서 및 GraphicsEngine 시각화 모듈을 통합 연동하는 Qt 호스트 애플리케이션 작업공간입니다.

---

## 📌 주요 기능
- **센서 모듈 통합**: `Camera` 및 `Gocator` 센서 모듈의 데이터 취득을 제어하고 통합 UI에 바인딩합니다.
- **의존성 분리**: 하위 렌더링 엔진(`GraphicsEngine`)과 장치 SDK(Pylon, GoPxL)가 직접 결합되지 않도록 각각의 어댑터를 호스트단에서 병합 컴파일하여 주입합니다.
- **스레드 안전 큐잉**: 워커 스레드에서 수신되는 grab 콜백 데이터를 `QMetaObject::invokeMethod`를 통해 안전하게 GUI 스레드로 전달해 렌더링합니다.
- **MDI workspace**: `MainWindow` hosts device windows in a branded MDI area with a neutral gray background and centered `BASLER_Logo.png` watermark.
- **Global logging**: `LogManager` captures Qt logs plus module `std::cout`/`std::cerr` logs into the System Logs dock and `lastlog.log`.
- **Shutdown safety**: MDI subwindows are deleted before `CameraSystem` destruction so device callbacks and camera ownership are cleaned up in order.

## 🛠️ 요구 사양 및 의존성
- **OS**: macOS / Windows
- **언어 표준**: C++20
- **의존 라이브러리**:
  - CMake 3.21+ / Qt 6.4+ (Widgets, OpenGL, Xml, Concurrent)
  - VTK (Qt OpenGL Native Widget 활성화 필요)
  - Basler Pylon SDK / LMI GoPxL SDK

## 🚀 빌드 및 실행
```bash
# Clone with modules
git clone --recurse-submodules git@github.com:minu-park/Playground.git

# Or initialize modules after cloning
git submodule update --init --recursive

# CMake 프로젝트 구성
cmake -S . -B build/cmake-build-debug

# 빌드
cmake --build build/cmake-build-debug --target Playground -j 8

# 실행
./build/cmake-build-debug/Playground
```

## 📂 디렉토리 구조
- `src/main.cpp`: 호스트 앱 진입점 및 센서-엔진 간 콜백 바인딩
- `src/MainWindow.cpp`: MDI workspace, global log dock, device creation, and parent-level shutdown ordering
- `src/DeviceWindow.cpp`: device-specific control dock and GraphicsEngine callback routing
- `modules/`: 독립 서브모듈 디렉토리 (Camera, Gocator, GraphicsEngine, Resources)
- `docs/`: 구조 및 개발 가이드 문서
- `docs/IMAGING_CONTROLLER_PLAN.md`: staged Camera imaging lifecycle and processing pipeline plan
