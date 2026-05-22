# 실시간 OpenCV/PCL 런타임 C++ 동적 컴파일 및 실행 시스템 제안

본 문서는 Playground MDI 구조 내에서 사용자가 직접 OpenCV C++ 코드를 작성하고, 이를 실시간으로 컴파일하여 영상 처리 파이프라인에 즉시 반영하는 **런타임 동적 컴파일 시스템(Runtime Dynamic Compilation System)** 구축에 대한 상세 제안 및 아키텍처 가이드입니다.

---

## 1. 아키텍처 핵심 개념

이 방식은 산업용 비전 소프트웨어(예: Cognex VisionPro, Halcon)의 스크립팅 엔진과 유사한 방식으로 작동합니다. 
C++의 고성능 강점을 유지하면서 빌드-실행 주기를 단축하기 위해 **공유 라이브러리(Dynamic Library, `.dylib`/`.so`) 동적 로딩** 기술을 활용합니다.

```mermaid
sequenceDiagram
    participant UI as 코드 에디터 (QTextEdit)
    participant FS as 파일 시스템 (Temporary C++ File)
    participant Proc as QProcess (clang++)
    participant Lib as QLibrary (dlopen)
    participant Loop as Grab Callback Loop

    UI->>FS: 1. 작성된 코드 저장 (dynamic_filter.cpp)
    UI->>Proc: 2. 컴파일 명령 실행
    Proc->>FS: 3. 컴파일 및 링킹 (dynamic_filter.dylib 생성)
    Proc-->>UI: 4. 컴파일 로그 피드백 (성공/실패 메세지)
    Note over UI,Lib: 컴파일 성공 시
    UI->>Lib: 5. 기존 라이브러리 언로드 및 신규 라이브러리 로드
    Lib-->>Loop: 6. 함수 포인터 갱신 (extern "C" process 함수)
    Loop->>Loop: 7. 새로운 알고리즘으로 실시간 이미지 가공 실행
```

---

## 2. 두 가지 접근 방식 비교: C++ 동적 컴파일 vs Python 스크립트

사용자 요구사항에 맞추어 아래 두 가지 대안을 검토할 수 있습니다.

| 비교 항목 | 대안 A: C++ 동적 컴파일 (추천) | 대안 B: Python 스크립트 임베딩 |
| :--- | :--- | :--- |
| **개념** | 코드를 `.cpp`로 저장 후 `clang++`로 컴파일, `.dylib` 로드 | Python 인터프리터를 내장하여 NumPy 배정 후 스크립트 실행 |
| **성능 (속도)** | **최상** (Native C++ 속도, 실시간 카메라 grab 루프 적합) | **보통** (Python GIL 및 데이터 포맷 변환 오버헤드 존재) |
| **구현 난이도** | **중** (`QProcess` + `QLibrary` 활용, 약 150라인) | **상** (Pybind11/NumPy C++ 연동 설정 복잡) |
| **안전성** | 코드 오류(예: 잘못된 포인터) 발생 시 앱 크래시 가능성 존재 | 스크립트 오류 시 예외 처리로 앱 안정성 유지 |
| **추천 이유** | 이미 개발 환경(Clang/CMake)이 구축되어 있고, 생산용 C++ 코드로 즉시 변환이 용이함 | - |

---

## 3. 세부 구현 아키텍처 (C++ 동적 컴파일 기준)

### A. 고정된 입력/출력 인터페이스 규격 정의
사용자가 복잡한 Qt/MDI 연동 코드를 짤 필요 없이, 순수 알고리즘에만 집중하도록 입출력 시그니처를 고정합니다.

```cpp
// 사용자가 코드 창에 입력할 코드 템플릿
#include <opencv2/opencv.hpp>

extern "C" {
    // 2D 이미지 처리용 인터페이스
    void processImage(const cv::Mat& src, cv::Mat& dst, double param1, double param2) {
        // 예시: 가우시안 블러 및 캐니 에지 검출
        cv::Mat gray, blurred;
        if (src.channels() == 3) {
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = src;
        }
        
        // UI 슬라이더에서 넘겨받는 파라미터 적용 테스트
        int kernelSize = (int(param1) % 2 == 0) ? int(param1) + 1 : int(param1);
        kernelSize = std::max(1, kernelSize);
        
        cv::GaussianBlur(gray, blurred, cv::Size(kernelSize, kernelSize), 0);
        cv::Canny(blurred, dst, param2, param2 * 3);
    }
}
```

### B. UI 구성 (Code Editor Dock Widget)
`DeviceWindow` 하단 혹은 우측에 `QDockWidget` 형태로 코딩창을 배치합니다.

1.  **Editor Area**: `QTextEdit`을 활용하고 기본 C++ 문법 강조(QSyntaxHighlighter) 적용.
2.  **Control Panel**:
    *   `Compile & Apply` 버튼
    *   `Parameter 1`, `Parameter 2` 실시간 조절 슬라이더 (사용자 코드의 `param1`, `param2`에 매핑되어 컴파일 없이 조절 가능)
3.  **Log Console**: `QTextEdit` (Read-only)를 배치하여 `clang++` 빌드 에러/경고 실시간 출력.

### C. 호스트 코드에서의 빌드 및 로딩 파이프라인
`QProcess`를 이용해 백그라운드에서 빌드를 수행합니다.

```cpp
// 1. 코드 파일 저장
QFile file("scratch/dynamic_filter.cpp");
if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();
}

// 2. QProcess를 통한 백그라운드 컴파일
QProcess* compiler = new QProcess(this);
QStringList arguments;
arguments << "-O2" << "-shared" << "-fPIC" << "-std=c++20"
          << "-I/opt/homebrew/include/opencv4" // OpenCV 헤더 위치 (macOS 예시)
          << "-L/opt/homebrew/lib"
          << "-lopencv_core" << "-lopencv_imgproc"
          << "scratch/dynamic_filter.cpp" 
          << "-o" << "scratch/dynamic_filter.dylib";

compiler->start("clang++", arguments);
connect(compiler, &QProcess::finished, this, [this, compiler](int exitCode) {
    if (exitCode == 0) {
        logConsole->append("Compilation Succeeded!");
        loadDynamicLibrary();
    } else {
        logConsole->append("Compilation Failed:\n" + compiler->readAllStandardError());
    }
    compiler->deleteLater();
});
```

### D. 동적 함수 로딩 및 이미지 처리 적용
`QLibrary`를 통해 컴파일된 `.dylib`를 로드하고 가로챕니다.

```cpp
typedef void (*ProcessFunc)(const cv::Mat&, cv::Mat&, double, double);
ProcessFunc activeFilterFunction = nullptr;
QLibrary* currentLib = nullptr;

void loadDynamicLibrary() {
    // 1. 기존 라이브러리 언로드 (파일 락 해제)
    if (currentLib) {
        activeFilterFunction = nullptr;
        currentLib->unload();
        delete currentLib;
        currentLib = nullptr;
    }
    
    // 2. 신규 라이브러리 로드
    currentLib = new QLibrary("scratch/dynamic_filter.dylib");
    if (currentLib->load()) {
        activeFilterFunction = (ProcessFunc)currentLib->resolve("processImage");
        if (!activeFilterFunction) {
            logConsole->append("Error: 'processImage' symbol not found.");
        }
    } else {
        logConsole->append("Load error: " + currentLib->errorString());
    }
}
```

---

## 4. 실시간 렌더링 루프 연동
카메라 프레임 콜백 함수에서 `activeFilterFunction`이 로드되어 있다면 이를 통과시킨 뒤 `GraphicsEngine`에 전달합니다.

```cpp
// 카메라 grab 콜백 내부
const auto grabCallbackId = camera->registerGrabCallback(
    [this](const CPylonImage& pylonImage, size_t) {
        cv::Mat rawMat = convertPylonToMat(pylonImage); // 변환
        cv::Mat processedMat;

        if (activeFilterFunction) {
            // 사용자가 작성한 OpenCV 필터 실행
            activeFilterFunction(rawMat, processedMat, slider1Val, slider2Val);
        } else {
            processedMat = rawMat;
        }

        QImage finalImage = convertMatToQImage(processedMat);
        QMetaObject::invokeMethod(graphicsEngine, [graphicsEngine, finalImage]() {
            graphicsEngine->setImage(finalImage);
        });
    }
);
```

---

## 5. 결론 및 제안

*   **실현 가능성**: **매우 높음**. macOS 환경에서 `clang++`가 기본 제공되므로, 링커 세팅만 잘 잡아준다면 가벼운 코드로 강력한 샌드박스 코딩창을 구축할 수 있습니다.
*   **복잡도 평가**: 시스템 자체는 단순 추상화 수준으로 150줄 내외로 코어 로직 작성이 가능하지만, 사용자가 세그멘테이션 폴트 등을 낼 경우 전체 앱이 꺼지는 부분에 대해 주의(예외 처리 또는 컴파일 안전 가이드라인 제공)가 필요합니다.
*   **제안**:
    1.  이번 마일스톤에 **간단한 C++ 스크립팅/컴파일 모듈**을 시범적으로 포함하고, OpenCV 2D 필터(Canny, Blur 등)를 타겟으로 동작하도록 구성합니다.
    2.  PCL의 경우 헤더 템플릿 컴파일 시간이 20-30초 이상 걸려 실시간 타이핑 테스트용으로는 부적합하므로, 3D 필터는 하드코딩된 빌트인 필터 선택 방식을 사용하고 **2D OpenCV 필터만 코딩창을 적용**하는 방식을 제안합니다.
