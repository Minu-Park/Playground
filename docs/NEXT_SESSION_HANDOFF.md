# Next Session Handoff

## 지시
- 이 문서는 다음 작업 세션의 시작점이다.
- 먼저 부모 저장소와 `modules/Camera`, `modules/GraphicsEngine` 상태를 각각 확인한다.
- 이 문서와 함께 게시된 부모/모듈 커밋을 기준으로 이어서 검증/수정한다.
- 다음 목표는 Stereo mini 컬러 점군 실장치 확인과 registration profile 정책 확정이다.

## 현재 상태
- 기준일: 2026-05-26.
- Camera module 변경은 `f73be55` (`Add stereo color 3D scene adapter support`)로 `origin/main`에 게시했다.
- GraphicsEngine module 변경은 `c5e2806` (`Support color images and RGB range point clouds`)로 `origin/main`에 게시했다.
- 부모 repo는 이 문서와 함께 두 submodule pointer, integration code, 구조/상태 문서 정리를 게시한다.
- 마지막 검증은 `git diff --check` 3곳 통과 및 부모 `Playground` 타깃 configure/build 통과다.
- 게시 변경은 기능 구현, 책임 경계 정리, 문서 삭제/대체를 함께 포함하므로 후속 변경도 저장소별 소유권을 유지한다.

## 완료된 구현

### Camera module
- `PylonScene3DAdapter` facade 추가: Blaze compatibility, Stereo mini direct XYZ, Stereo ace disparity reconstruction 라우팅.
- `PylonScene3DProfile` 분리: adapter가 `Camera` 클래스 내부 profile 타입에 의존하지 않도록 정리.
- Stereo mini stream: `Source3` intensity/color와 `Source1` range 사용, 기본 mapping은 `None`.
- Stereo mini multipart stride 방어: reported stride가 component buffer 크기와 맞지 않으면 packed stride 사용.
- Stereo mini `RGBA8` 표시: alpha를 opacity로 해석하지 않고 RGB만 사용.
- 점군 색상 생성: profile 문자열 판정이 아니라 실제 frame의 `RGB8/RGBA8` component 기준.

### GraphicsEngine module
- `GraphicsScene3D`에 optional `colorImage`와 registration metadata 추가.
- `RangeFrame`에 organized `rgb` 추가하고 `RangeFramePointCloudBuilder`가 이를 점군 색상으로 전달.
- 컬러 scene 최초 진입 시 `Image2D` 표시 유지, PointCloud 모드에서는 RGB 포함 range frame 요청.
- 중복 계약이었던 `GraphicsScene3DRequest::includeColorImage` 제거; `content` bitmask가 단일 기준.

### Parent app
- `CameraImagingController`가 `PylonScene3DAdapter`와 `PylonScene3DProfile`을 사용하도록 연결.
- OpenCV hardcoded `/opt/opencv` 탐색 제거; optional discovery로 전환.
- 완료/중복/오류 문서를 제거하고 현재 상태 문서로 교체.

## 실장치에서 확인된 사실
- 연결 장치 목록에서 `Stereo mini STM-501u` (`BaslerGTC/Basler/Stereo_mini`)가 확인되었다.
- Stereo mini `Intensity`는 실제 payload에서 `RGBA8` 컬러 component로 수신되었다.
- component가 보고한 stride/padding은 raw buffer size와 불일치할 수 있어 기존 줄 단위 깨짐 원인이 되었다.
- `RGBA8` alpha=0 픽셀이 존재해 alpha를 투명도로 표시하면 white pepper/흰 배경 형태가 발생할 수 있다.
- 사용자는 최종적으로 PointCloud가 컬러가 아니라 흰색으로만 보인다고 보고했다. 이후 RGB 생성 gate 제거와 빌드는 완료했지만, 해당 수정 후 사용자 화면 재확인은 아직 받지 못했다.

## 아직 확정하지 않은 사항
- Stereo mini 기본 mapping을 원본 color 보존용 `None`으로 유지할지, 점군 등록 우선 `ColorOntoDepth` profile을 제공/기본화할지.
- hardware-registered color와 raw color를 UI에서 어떻게 명시적으로 선택/표시할지.
- Stereo ace 실장치 및 mono model 회귀 결과.
- IR/confidence/error 채널 노출 범위.
- `PylonScene3DAdapter.cpp`의 buffer decode, geometry decode, scene assembly 분리 시점.
- `GraphicsEngine::applyScene3D()`의 state transition, backend apply, chrome update 책임 분리.
- Dynamic processing의 `create_node`/`process_image` ABI 통일 및 플랫폼별 compiler/shared-library 정책.

## 다음 세션 작업 순서
1. `git status --short`를 부모와 두 변경 모듈에서 각각 확인한다.
2. 앱을 재시작하거나 카메라를 재연결한 뒤 Stereo mini `PointCloud3D`에서 RGB가 실제로 보이는지 확인한다.
3. 흰색이 계속되면 `RangeFrame::rgb` 크기와 `PointCloudData::rgb` 크기가 frame별로 채워지는 지점을 진단한다.
4. RGB가 보이면 `None`과 `ColorOntoDepth`를 같은 장면에서 비교하고, default/profile UX 결정 전에 결과와 FPS를 기록한다.
5. 실장치 기준이 확정된 뒤에만 adapter 내부 분리 또는 registration UI를 진행한다.
6. 기능 검증에 따른 후속 변경은 Camera, GraphicsEngine, 부모의 별도 커밋 경계를 유지한다.

## 첫 검증 명령
```bash
git status --short
git -C modules/Camera status --short
git -C modules/GraphicsEngine status --short
git diff --check
git -C modules/Camera diff --check
git -C modules/GraphicsEngine diff --check
cmake -S . -B build/cmake-build-debug
cmake --build build/cmake-build-debug --target Playground -j 8
```

## 관련 문서
- `docs/STEREO_3D_CAMERA_INTEGRATION.md`: stereo 데이터 계약과 남은 결정.
- `docs/STRUCTURAL_REVIEW.md`: 책임 경계 부채 목록.
- `docs/DYNAMIC_PROCESSING_STATUS.md`: processing ABI/플랫폼 경계.
- `modules/GraphicsEngine/docs/GRAPHICS_ENGINE_STRUCTURE.md`: neutral scene/render 소유권.
