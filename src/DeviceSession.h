#pragma once
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include "Camera.h"
#include "Gocator.h"

class GraphicsEngine;
class QDockWidget;
class CameraSystem;
class QCameraWidget;
class QGocatorWidget;
class QProcessingWidget;
class QStaticImageControlWidget;
class QMdiSubWindow;

#include <memory>

class AbstractImagingController;
class GraphicsEngineSink;

class DeviceSession : public QMainWindow {
    Q_OBJECT
public:
    explicit DeviceSession(Camera* camera, CameraSystem* cameraSystem, QWidget* parent = nullptr);
    explicit DeviceSession(Gocator* gocator, QWidget* parent = nullptr);
    explicit DeviceSession(const QStringList& filePaths, QWidget* parent = nullptr);
    ~DeviceSession() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    Q_INVOKABLE void notifyManualResizeFinished();

    void setSubWindow(QMdiSubWindow* subWin) { _subWindow = subWin; }

private:
    void initCommon();
    void initCamera();
    void initGocator();
    void initStaticImage(const QStringList& filePaths);
    void setControlWidget(QWidget* widget, const QString& title);
    void createProcessingDock();
    void setupViewMenu();
    void adjustSessionWidth();

    QMdiSubWindow* _subWindow = nullptr;
    GraphicsEngine* _graphicsEngine = nullptr;
    QDockWidget* _controlDock = nullptr;
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

    bool _processingDockWasDockedVisible = false;
    bool _controlDockWasDockedVisible = false;
    int _undockedMainWindowWidth = 0;

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void updateProcessingDockLayoutState();
    void updateControlDockLayoutState();
};
