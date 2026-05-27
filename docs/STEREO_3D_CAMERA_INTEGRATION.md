# Basler Stereo 3D Integration Status

## 현재 계약
- `modules/Camera`가 pylon 장치 연결, stream 설정, `ready()` backpressure와 `PylonScene3DProfile` snapshot을 소유한다.
- `PylonScene3DAdapter`가 pylon multipart frame을 `GraphicsScene3D`로 변환한다.
- `modules/GraphicsEngine`는 센서 독립 `colorImage`, registration metadata, organized `RangeFrame::rgb`만 수신한다.
- Playground controller는 callback, adapter 호출, pipeline, GUI-thread sink 연결만 담당한다.

## 구현 결과
- Blaze는 기존 direct XYZ 변환 경로를 facade 아래에서 유지한다.
- Stereo mini는 `Source3` RGB/RGBA와 `Range/Coord3D_ABC32f`를 사용한다.
- Stereo ace는 `Intensity`와 `Disparity/Coord3D_C16`, calibration snapshot으로 XYZ를 구성한다.
- 컬러 장면은 최초 표시에서 `Image2D`를 유지하며 PointCloud 전환 시 `RangeFrame::rgb`를 사용한다.
- Stereo mini 실장치에서 component reported stride가 raw data size와 불일치함을 확인해, adapter가 data size로 stride를 검증한다.
- Stereo mini `RGBA8` alpha는 투명도로 쓰지 않고 RGB 바이트만 표시와 점군 색에 사용한다.
- 점군 색 유무는 profile 문자열이 아니라 수신 frame의 실제 `RGB8/RGBA8` component로 판정한다.

## 실장치 확인 기준
- 연결 장치에서 `Stereo mini STM-501u` (`BaslerGTC/Basler/Stereo_mini`)가 확인되었다.
- Stereo mini `Intensity`는 실제 payload에서 `RGBA8` color component로 수신되었다.
- Component가 보고한 stride/padding과 raw buffer size 불일치가 줄 단위 깨짐의 원인이었으며, packed-stride fallback으로 방어한다.
- `RGBA8` alpha=0 픽셀이 존재하므로 alpha를 opacity로 처리하면 흰 배경 형태의 표시 오류가 발생할 수 있다.
- RGB 생성 gate 수정 이후 `PointCloud3D`에서 실제 색 표시 재확인은 아직 남아 있다.

## 남은 결정
- 원본 color 해상도 보존(`None`)과 hardware registration(`ColorOntoDepth`) 중 기본 profile 정책.
- IR/confidence/error 채널과 profile 선택 UI 범위.
- Stereo ace 실장치 및 mono 모델 회귀 검증.
- Windows/Linux producer 배포 검증과 macOS 비지원 처리.

## 검증
1. pylon Viewer/공식 sample과 depth, RGB image, color point cloud를 비교한다.
2. Stereo mini mapping profile별 대응 결과와 FPS를 측정한다.
3. grab/stop/reconnect/session close/app quit 및 기존 Blaze/Gocator/2D 경로 회귀를 확인한다.
4. 변경 저장소별 `git diff --check`와 부모 `build/` 타깃 빌드를 수행한다.

## 공식 자료
- [pylon 3D Supplementary Packages](https://docs.baslerweb.com/pylon-3d)
- [Stereo mini Supplementary Package](https://docs.baslerweb.com/pylon-supplementary-package-for-stereo-mini)
- [Stereo mini C++ Programmer's Guide](https://docs.baslerweb.com/cpp-prog-guide-%28stereo-mini%29)
- [Stereo ace Supplementary Package](https://docs.baslerweb.com/pylon-supplementary-package-for-stereo-ace)
- [Stereo ace Pixel Format](https://docs.baslerweb.com/pixel-format-%28stereo-ace%29)
- [Stereo ace Measurement Processing](https://docs.baslerweb.com/processing-measurement-results-%28stereo-ace%29)
