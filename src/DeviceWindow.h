#pragma once
#include <QMainWindow>
#include "Camera.h"
#include "Gocator.h"

class GraphicsEngine;
class QDockWidget;
class CameraSystem;
class QCameraWidget;
class QGocatorWidget;
class QProcessingWidget;
class QStaticImageControlWidget;

#include <memory>

class AbstractImagingController;
class GraphicsEngineSink;

class DeviceWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DeviceWindow(Camera* camera, CameraSystem* cameraSystem, QWidget* parent = nullptr);
    explicit DeviceWindow(Gocator* gocator, QWidget* parent = nullptr);
    explicit DeviceWindow(const QStringList& filePaths, QWidget* parent = nullptr);
    ~DeviceWindow() override;

private:
    void initCommon();
    void initCamera();
    void initGocator();
    void initStaticImage(const QStringList& filePaths);

    GraphicsEngine* _graphicsEngine;
    QDockWidget* _controlDock;
    QDockWidget* _processingDock = nullptr;

    Camera* _camera;
    CameraSystem* _cameraSystem;
    Gocator* _gocator;

    QCameraWidget* _cameraWidget = nullptr;
    QGocatorWidget* _gocatorWidget = nullptr;
    QProcessingWidget* _processingWidget = nullptr;
    QStaticImageControlWidget* _staticImageWidget = nullptr;

    std::unique_ptr<AbstractImagingController> _controller;
    GraphicsEngineSink* _sink = nullptr;
};
