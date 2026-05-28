#include "Controller/CameraImagingController.h"
#include "Controller/GraphicsEngineSink.h"
#include "Pipeline/ProcessingFrame.h"
#include "engine/GraphicsEngine.h"
#include "PylonScene3DAdapter.h"
#include "Utility/Qt/QtConverter.h"
#include <QPointer>
#include <QDebug>
#include <exception>

namespace {

class CameraReadyGuard {
public:
    explicit CameraReadyGuard(Camera* camera)
        : _camera(camera) {}

    ~CameraReadyGuard() {
        if (_camera) {
            _camera->ready();
        }
    }

private:
    Camera* _camera = nullptr;
};

} // namespace

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
            Camera* camera = _camera;
            CameraReadyGuard ready(camera);
            if (!camera) {
                return;
            }
            if (!_sink) {
                return;
            }

            try {
                QImage image = convertPylonImageToQImage(pylonImage);

                if (!image.isNull()) {
                    ProcessingFrame frame;
                    frame.payload = std::move(image);
                    frame.frameSeq = _frameSeq++;

                    _pipeline.run(frame);

                    if (std::holds_alternative<QImage>(frame.payload)) {
                        _sink->enqueueImage(std::get<QImage>(frame.payload));
                    }
                }
            } catch (const std::exception& e) {
                qWarning() << "[CameraImagingController] 2D frame processing failed:" << e.what();
            } catch (...) {
                qWarning() << "[CameraImagingController] 2D frame processing failed with an unknown exception.";
            }
        });

    // 3. 3D Grab callback
    const PylonScene3DAdapter scene3DAdapter;
    _grab3DCallbackId = _camera->registerGrab3DCallback(
        [this, scene3DAdapter](const Pylon::CPylonDataContainer& container, size_t) {
            Camera* camera = _camera;
            CameraReadyGuard ready(camera);
            if (!camera) {
                return;
            }
            if (!_sink) {
                return;
            }

            GraphicsEngine* engine = _sink->engine();
            if (!engine) {
                return;
            }

            try {
                const GraphicsScene3DRequest request = engine->scene3DRequest();
                const PylonScene3DProfile profile = camera->scene3DProfile();
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
            } catch (const std::exception& e) {
                qWarning() << "[CameraImagingController] 3D frame processing failed:" << e.what();
            } catch (...) {
                qWarning() << "[CameraImagingController] 3D frame processing failed with an unknown exception.";
            }
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
