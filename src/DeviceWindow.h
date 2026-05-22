#pragma once
#include <QMainWindow>
#include "Camera.h"
#include "Gocator.h"

class GraphicsEngine;
class QDockWidget;
class CameraSystem;
class QCameraWidget;
class QGocatorWidget;

#ifdef HAS_OPENCV
#include <opencv2/opencv.hpp>
#include <mutex>
using ProcessFunc = void (*)(const cv::Mat&, cv::Mat&, double, double);
#else
using ProcessFunc = void*;
#endif

class DeviceWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DeviceWindow(Camera* camera, CameraSystem* cameraSystem, QWidget* parent = nullptr);
    explicit DeviceWindow(Gocator* gocator, QWidget* parent = nullptr);
    ~DeviceWindow() override;

private:
    void initCommon();
    void initCamera();
    void initGocator();

    GraphicsEngine* _graphicsEngine;
    QDockWidget* _controlDock;

    Camera* _camera;
    CameraSystem* _cameraSystem;
    Gocator* _gocator;

    QCameraWidget* _cameraWidget = nullptr;
    QGocatorWidget* _gocatorWidget = nullptr;

#ifdef HAS_OPENCV
    std::mutex _filterMutex;
    ProcessFunc _activeFilter = nullptr;
    double _param1 = 5.0;
    double _param2 = 50.0;
#endif

    Camera::CallbackId _grabCallbackId = 0;
    Camera::CallbackId _grab3DCallbackId = 0;
    Gocator::CallbackId _gocatorCallbackId = 0;
};
