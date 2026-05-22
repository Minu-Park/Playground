# Playground

> Basler 카메라, LMI Gocator 3D 센서 및 generic 2D/3D 시각화 라이브러리(GraphicsEngine)를 결합하여 구성한 Qt 데모 호스트 애플리케이션 작업공간(Workspace)입니다.

이 프로젝트는 독립된 4개의 서브모듈(`modules/*`)을 유기적으로 엮어 실시간 2D/3D 센서 획득 데이터를 시각화하는 통합 오케스트레이션 역할을 담당합니다.

---

## 📌 주요 특징 (Key Features)
- **센서 모듈 통합 오케스트레이션**: `Camera` 모듈의 2D/3D 데이터 취득 및 `Gocator` 모듈의 3D 데이터 취득을 제어하고 통합 UI에 바인딩합니다.
- **의존성 디커플링 (SDK Adapters)**: 하위 렌더링 엔진인 `GraphicsEngine`에 하드웨어 제조사 SDK(Pylon, GoPxL)가 직결되는 것을 방지합니다. 호스트 앱인 Playground가 각각의 어댑터(`BlazeScene3DAdapter`, `GocatorDataSetScene3DAdapter`)를 빌드 시점에 주입하도록 설계되었습니다.
- **스레드 안전한 GUI 큐잉**: 비동기 워커 스레드에서 발생하는 하드웨어 grab 콜백을 Qt GUI 스레드로 안전하게 큐잉(`QMetaObject::invokeMethod`)하여 프레임 렌더링을 처리합니다.
- **전역 자원 통합 관리**: 스타일시트와 아이콘 에셋은 `Resources` 모듈에서 독점적으로 관리하며, 애플리케이션 시작 시 `Resources::installResources(app)` 호출을 통해 통합 반영됩니다.

## 🛠️ 요구 사양 및 의존성 (Prerequisites & Dependencies)
- **OS**: macOS / Windows
- **언어 표준**: C++20 (호스트 프로젝트 및 전체 모듈 공통)
- **필수 의존성**:
  - **CMake**: 버전 3.21 이상
  - **Qt6**: 버전 6.4 이상 (Widgets, OpenGL, Xml, Concurrent 구성요소 필수)
  - **VTK**: Qt 컴포넌트(`QVTKOpenGLNativeWidget`)를 지원하는 라이브러리 (GraphicsEngine 3D 렌더링용)
  - **Basler Pylon SDK**: `Camera` 취득 라이브러리 의존성
  - **LMI GoPxL SDK**: `Gocator` 취득 라이브러리 의존성 (로컬 경로 설정 필요)

## 🚀 시작하기 (Quick Start & Build)

### 1. 빌드 방법
상위 디렉토리에서 디버그 빌드를 구성하고 타겟을 빌드합니다.

```bash
# CMake 프로젝트 구성
cmake -S . -B build/cmake-build-debug

# Playground 호스트 애플리케이션 빌드
cmake --build build/cmake-build-debug --target Playground -j 8
```

### 2. 실행 방법
실행 파일 작동 시 Gocator SDK 라이브러리 공유 디렉토리에 대한 dynamic link 설정을 확인하십시오.
```bash
./build/cmake-build-debug/Playground
```

---

## 📂 디렉토리 구조 (Directory Structure)

```text
Playground/
├── CMakeLists.txt        # 호스트 앱 CMake 진입점 및 모듈 바인딩
├── AGENTS.md             # 에이전트 코딩 룰 및 경계 정의 가이드
├── src/
│   └── main.cpp          # 호스트 앱 메인 소스 및 콜백-렌더링 큐 바인딩
├── docs/
│   ├── STRUCTURE.md      # 프로젝트 아키텍처 및 모드 전환 흐름 구조도
│   └── DEVELOPMENT_GUIDE.md # 개발 우선순위 및 기본 검증 체크리스트
└── modules/
    ├── Camera/           # Basler pylon 카메라 연동 모듈 (독립 Git)
    ├── Gocator/          # LMI Gocator 3D 제어 모듈 (독립 Git)
    ├── GraphicsEngine/   # 2D/3D 공용 VTK 렌더링 모듈 (독립 Git)
    └── Resources/        # 공용 QSS 스타일 및 아이콘 리소스 모듈 (독립 Git)
```

---

## ⚠️ 아키텍처 규칙 및 제약 (Boundaries & Rules)
- **독립성 유지**: `modules/` 산하의 서브모듈들은 각각 독립적인 Git 저장소입니다. 소스 수정 및 커밋은 해당 모듈 저장소 내부에서 개별적으로 완료해야 합니다.
- **인터페이스 분리**: 호스트 애플리케이션은 각 모듈의 공개 API(Public Header) 및 제공 어댑터 클래스만을 연동하며, 모듈 내부의 private 구현 파일을 임의로 복제하거나 참조해서는 안 됩니다.
- **리소스 중앙화**: 모든 스타일시트 개정, 테마 선택자 변경, 신규 아이콘 추가 등 비주얼 요소는 호스트나 개별 장치 모듈이 아닌 `modules/Resources` 내부의 자원으로 추가 및 수정되어야 합니다.

## 📝 라이선스 (License)
본 프로젝트는 독점 상용 라이선스를 따르며 무단 배포와 상용 이용이 제한됩니다.
