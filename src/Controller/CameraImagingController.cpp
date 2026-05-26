#include "Controller/CameraImagingController.h"
#include "Controller/GraphicsEngineSink.h"
#include "Pipeline/ProcessingFrame.h"
#include "engine/GraphicsEngine.h"
#include "PylonScene3DAdapter.h"
#include "Utility/Qt/QtConverter.h"
#include <QPointer>
#include <QDebug>

#ifdef HAS_OPENCV
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

CameraImagingController::CameraImagingController(Camera* camera, QObject* parent)
    : AbstractImagingController(parent), _camera(camera) {
    registerCallbacks();
}

CameraImagingController::~CameraImagingController() {
    stop();
    deregisterCallbacks();
}

void CameraImagingController::start() {
    if (_camera) {
        _isGrabbing = true;
        _camera->grab();
    }
}

void CameraImagingController::stop() {
    if (_camera) {
        _camera->stop();
        _isGrabbing = false;
    }
}

bool CameraImagingController::isGrabbing() const {
    return _isGrabbing;
}

void CameraImagingController::setSink(GraphicsEngineSink* sink) {
    _sink = sink;
}

#ifdef HAS_OPENCV
void CameraImagingController::setFilter(ProcessFunc filter) {
    std::lock_guard<std::mutex> lock(_filterMutex);
    _activeFilter = filter;
}

void CameraImagingController::setParameters(double p1, double p2) {
    std::lock_guard<std::mutex> lock(_filterMutex);
    _param1 = p1;
    _param2 = p2;
}
#endif

void CameraImagingController::registerCallbacks() {
    if (!_camera) return;

    // 1. Status callback
    _statusCallbackId = _camera->registerStatusCallback([this](Camera::Status status, bool on) {
        if (status == Camera::GrabbingStatus) {
            _isGrabbing = on;
        }
    });

    // 2. 2D Grab callback
    _grabCallbackId = _camera->registerGrabCallback(
        [this](const CPylonImage& pylonImage, size_t) {
            if (!_sink) {
                _camera->ready();
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
                ProcessingFrame frame;
                frame.payload = std::move(image);
                frame.frameSeq = _frameSeq++;

                _pipeline.run(frame);

                if (std::holds_alternative<QImage>(frame.payload)) {
                    _sink->enqueueImage(std::get<QImage>(frame.payload));
                }
            }
            _camera->ready();
        });

    // 3. 3D Grab callback
    const PylonScene3DAdapter scene3DAdapter;
    _grab3DCallbackId = _camera->registerGrab3DCallback(
        [this, scene3DAdapter](const Pylon::CPylonDataContainer& container, size_t) {
            if (!_sink) {
                _camera->ready();
                return;
            }

            GraphicsEngine* engine = _sink->engine();
            if (!engine) {
                _camera->ready();
                return;
            }

            const GraphicsScene3DRequest request = engine->scene3DRequest();
            const PylonScene3DProfile profile = _camera->scene3DProfile();
            auto scene = scene3DAdapter.convert(container, request, profile);
            if (scene.has_value()) {
                ProcessingFrame frame;
                frame.payload = std::move(*scene);
                frame.frameSeq = _frameSeq++;

                _pipeline.run(frame);

                if (std::holds_alternative<GraphicsScene3D>(frame.payload)) {
                    _sink->enqueueScene3D(std::move(std::get<GraphicsScene3D>(frame.payload)));
                }
            }
            _camera->ready();
        });
}

void CameraImagingController::deregisterCallbacks() {
    if (!_camera) return;

    if (_statusCallbackId != 0) {
        _camera->deregisterStatusCallback(_statusCallbackId);
        _statusCallbackId = 0;
    }
    if (_grabCallbackId != 0) {
        _camera->deregisterGrabCallback(_grabCallbackId);
        _grabCallbackId = 0;
    }
    if (_grab3DCallbackId != 0) {
        _camera->deregisterGrab3DCallback(_grab3DCallbackId);
        _grab3DCallbackId = 0;
    }
}
