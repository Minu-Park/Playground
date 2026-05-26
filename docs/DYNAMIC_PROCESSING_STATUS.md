# Dynamic Processing Status

## 현재 상태
- `ProcessingPipeline`과 `ProcessingRegistry`가 2D processing node 실행을 소유한다.
- `DynamicLibraryLoader`와 `QProcessingWidget`의 C++ compile/apply UI가 존재한다.
- `QProcessingWidget`에는 pipeline/scripting tab, dynamic parameter rendering, compiler output, OpenCV header 기반 completion과 C++ highlighting이 구현되어 있다.
- OpenCV는 CMake에서 optional dependency이며, 발견된 빌드에서만 `HAS_OPENCV`와 live compile UI가 활성화된다.
- Scene3D processing semantics는 아직 정의되지 않았다.

## 확인된 경계 부채
- `DynamicLibraryLoader::createNode()`는 `create_node` 심볼을 기대하지만 `QProcessingWidget`은 `process_image`를 직접 resolve하고 별도 wrapper node를 만든다.
- `QProcessingWidget`이 compiler 실행, scratch 파일, library suffix와 ABI adapter까지 함께 소유한다.
- 현재 compile command는 `clang++`, `-fPIC`, `.dylib`에 고정되어 Windows/Linux/macOS 공통 계약이 아니다.
- UI 기능 완성과 별개로 위 ABI/플랫폼 경계는 아직 해결되지 않았다.

## 정리 방향
1. 단일 dynamic-node ABI를 먼저 결정한다.
2. compile/load 플랫폼 정책을 widget 밖 service로 추출한다.
3. 플랫폼별 compiler와 shared-library suffix를 지원하기 전에는 live compile을 검증 완료 기능으로 표시하지 않는다.
4. Scene3D processing은 별도 데이터 보존 계약을 정한 뒤 활성화한다.

## 유지 규칙
- Camera live 경로의 `ready()` backpressure를 우회하는 queue를 추가하지 않는다.
- processing 실패 시 frame 처리 결과와 permit 반환을 명시적으로 유지한다.
