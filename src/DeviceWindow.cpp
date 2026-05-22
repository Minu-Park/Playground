#include "DeviceWindow.h"
#include "engine/GraphicsEngine.h"
#include "Utility/Qt/QCameraWidget.h"
#include "Utility/Qt/QGocatorWidget.h"
#include "Utility/Qt/QtConverter.h"
#include "BlazeScene3DAdapter.h"
#include "GocatorDataSetScene3DAdapter.h"
#include "CameraSystem.h"
#include <QDockWidget>
#include <QLabel>
#include <QPointer>
#include <QMetaObject>

#ifdef HAS_OPENCV
#include <opencv2/opencv.hpp>

namespace {
cv::Mat convertPylonImageToMat(const CPylonImage& pylonImage) {
    if (!pylonImage.IsValid()) return cv::Mat();

    int width = (int)pylonImage.GetWidth();
    int height = (int)pylonImage.GetHeight();
    const void* buffer = pylonImage.GetBuffer();
    auto pixelType = pylonImage.GetPixelType();
    
    if (pixelType == Pylon::PixelType_Mono8) {
        return cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer)).clone();
    } else if (pixelType == Pylon::PixelType_RGB8packed) {
        cv::Mat rgbMat(height, width, CV_8UC3, const_cast<void*>(buffer));
        cv::Mat bgrMat;
        cv::cvtColor(rgbMat, bgrMat, cv::COLOR_RGB2BGR);
        return bgrMat;
    } else if (pixelType == Pylon::PixelType_BGR8packed) {
        return cv::Mat(height, width, CV_8UC3, const_cast<void*>(buffer)).clone();
    }
    return cv::Mat();
}

QImage convertMatToQImage(const cv::Mat& mat) {
    if (mat.empty()) return QImage();
    
    if (mat.type() == CV_8UC1) {
        QImage image(mat.data, mat.cols, mat.rows, (int)mat.step, QImage::Format_Grayscale8);
        return image.copy();
    } else if (mat.type() == CV_8UC3) {
        QImage image(mat.data, mat.cols, mat.rows, (int)mat.step, QImage::Format_BGR888);
        return image.copy();
    } else if (mat.type() == CV_8UC4) {
        QImage image(mat.data, mat.cols, mat.rows, (int)mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    return QImage();
}
} // namespace
#endif

DeviceWindow::DeviceWindow(Camera* camera, CameraSystem* cameraSystem, QWidget* parent)
    : QMainWindow(parent), _camera(camera), _cameraSystem(cameraSystem), _gocator(nullptr) {
    initCommon();
    initCamera();
}

DeviceWindow::DeviceWindow(Gocator* gocator, QWidget* parent)
    : QMainWindow(parent), _camera(nullptr), _cameraSystem(nullptr), _gocator(gocator) {
    initCommon();
    initGocator();
}

DeviceWindow::~DeviceWindow() {
    // Perform cleanup for camera
    if (_camera) {
        if (_cameraWidget) {
            _cameraWidget->prepareForShutdown();
        }
        if (_grabCallbackId != 0) {
            _camera->deregisterGrabCallback(_grabCallbackId);
        }
        if (_grab3DCallbackId != 0) {
            _camera->deregisterGrab3DCallback(_grab3DCallbackId);
        }
        _camera->stop();

        if (_cameraSystem) {
            _cameraSystem->removeCamera(_camera);
        }
        delete _camera;
        _camera = nullptr;
    }

    // Perform cleanup for gocator
    if (_gocator) {
        if (_gocatorWidget) {
            _gocatorWidget->prepareForShutdown();
        }
        if (_gocatorCallbackId != 0) {
            _gocator->deregisterGrabCallback(_gocatorCallbackId);
        }
        _gocator->stop();
        _gocator->close();
        delete _gocator;
        _gocator = nullptr;
    }
}

void DeviceWindow::initCommon() {
    // 1. Create GraphicsEngine as central widget
    _graphicsEngine = new GraphicsEngine(this);
    setCentralWidget(_graphicsEngine);

    // 2. Create right dock (Control Panel placeholder)
    _controlDock = new QDockWidget(QStringLiteral("Device Control"), this);
    _controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    auto* dummyControl = new QLabel(QStringLiteral("Control Panel Placeholder"), _controlDock);
    dummyControl->setAlignment(Qt::AlignCenter);
    _controlDock->setWidget(dummyControl);
    addDockWidget(Qt::RightDockWidgetArea, _controlDock);
}

void DeviceWindow::initCamera() {
    if (!_camera) return;

    // Create camera widget and set it to control dock
    _cameraWidget = new QCameraWidget(_controlDock, _camera);
    _controlDock->setWidget(_cameraWidget);

    // Register real grab callbacks
    _grabCallbackId = _camera->registerGrabCallback(
        [this, target = QPointer<GraphicsEngine>(_graphicsEngine), camera = _camera](const CPylonImage& pylonImage, size_t) {
            if (target.isNull()) {
                camera->ready();
                return;
            }

            QImage image;
#ifdef HAS_OPENCV
            ProcessFunc filter = nullptr;
            double p1 = 0.0, p2 = 0.0;
            {
                std::lock_guard<std::mutex> lock(_filterMutex);
                filter = _activeFilter;
                p1 = _param1;
                p2 = _param2;
            }

            if (filter) {
                cv::Mat rawMat = convertPylonImageToMat(pylonImage);
                if (!rawMat.empty()) {
                    cv::Mat processedMat;
                    try {
                        filter(rawMat, processedMat, p1, p2);
                        if (!processedMat.empty()) {
                            image = convertMatToQImage(processedMat);
                        }
                    } catch (...) {
                        // Fail-safe in case dynamic user code has segment faults / logic errors
                    }
                }
            }
#endif

            if (image.isNull()) {
                image = convertPylonImageToQImage(pylonImage);
            }

            if (!image.isNull()) {
                QMetaObject::invokeMethod(target.data(),
                                          [target, image = std::move(image)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setImage(image);
                                              }
                                          },
                                          Qt::QueuedConnection);
            }
            camera->ready();
        });

    const BlazeScene3DAdapter scene3DAdapter;
    _grab3DCallbackId = _camera->registerGrab3DCallback(
        [target = QPointer<GraphicsEngine>(_graphicsEngine), camera = _camera, scene3DAdapter](
            const Pylon::CPylonDataContainer& container,
            size_t) {
            if (target.isNull())
            {
                camera->ready();
                return;
            }

            const GraphicsScene3DRequest request = target->scene3DRequest();
            auto scene = scene3DAdapter.convert(container, request);
            if (scene.has_value())
            {
                QMetaObject::invokeMethod(target.data(),
                                          [target, scene = std::move(*scene)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setScene3D(std::move(scene));
                                              }
                                          },
                                          Qt::QueuedConnection);
            }
            camera->ready();
        });
}

void DeviceWindow::initGocator() {
    if (!_gocator) return;

    // Create Gocator widget and set it to control dock
    _gocatorWidget = new QGocatorWidget(_controlDock, _gocator);
    _controlDock->setWidget(_gocatorWidget);

    const GocatorDataSetScene3DAdapter scene3DAdapter;
    _gocatorCallbackId = _gocator->registerGrabCallback(
        [target = QPointer<GraphicsEngine>(_graphicsEngine), gocator = _gocator, scene3DAdapter](
            const GoPxLSdk::GoDataSet& dataSet,
            size_t) {
            if (target.isNull())
            {
                return;
            }

            const GraphicsScene3DRequest request = target->scene3DRequest();
            auto scene = scene3DAdapter.convert(dataSet, request);
            if (scene.has_value())
            {
                QMetaObject::invokeMethod(target.data(),
                                          [target, scene = std::move(*scene)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setScene3D(std::move(scene));
                                              }
                                          },
                                          Qt::QueuedConnection);
            }
        });
}
