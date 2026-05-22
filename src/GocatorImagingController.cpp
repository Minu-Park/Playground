#include "GocatorImagingController.h"
#include "GraphicsEngineSink.h"
#include "ProcessingFrame.h"
#include "engine/GraphicsEngine.h"
#include "GocatorDataSetScene3DAdapter.h"
#include <QPointer>
#include <QDebug>

GocatorImagingController::GocatorImagingController(Gocator* gocator, QObject* parent)
    : AbstractImagingController(parent), _gocator(gocator) {
    registerCallbacks();
}

GocatorImagingController::~GocatorImagingController() {
    stop();
    deregisterCallbacks();
}

void GocatorImagingController::start() {
    if (_gocator) {
        _isGrabbing = true;
        _gocator->grab();
    }
}

void GocatorImagingController::stop() {
    if (_gocator) {
        _gocator->stop();
        _isGrabbing = false;
    }
}

bool GocatorImagingController::isGrabbing() const {
    return _isGrabbing;
}

void GocatorImagingController::setSink(GraphicsEngineSink* sink) {
    _sink = sink;
}

void GocatorImagingController::registerCallbacks() {
    if (!_gocator) return;

    // 1. Status callback
    _statusCallbackId = _gocator->registerStatusCallback([this](Gocator::Status status, bool on) {
        if (status == Gocator::GrabbingStatus) {
            _isGrabbing = on;
        }
    });

    // 2. Grab callback
    const GocatorDataSetScene3DAdapter scene3DAdapter;
    _grabCallbackId = _gocator->registerGrabCallback(
        [this, scene3DAdapter](const GoPxLSdk::GoDataSet& dataSet, size_t) {
            if (!_sink) return;

            GraphicsEngine* engine = _sink->engine();
            if (!engine) return;

            const GraphicsScene3DRequest request = engine->scene3DRequest();
            auto scene = scene3DAdapter.convert(dataSet, request);
            if (scene.has_value()) {
                ProcessingFrame frame;
                frame.payload = std::move(*scene);
                frame.frameSeq = _frameSeq++;

                _pipeline.run(frame);

                if (std::holds_alternative<GraphicsScene3D>(frame.payload)) {
                    _sink->enqueueScene3D(std::move(std::get<GraphicsScene3D>(frame.payload)));
                }
            }
        });
}

void GocatorImagingController::deregisterCallbacks() {
    if (!_gocator) return;

    if (_statusCallbackId != 0) {
        _gocator->deregisterStatusCallback(_statusCallbackId);
        _statusCallbackId = 0;
    }
    if (_grabCallbackId != 0) {
        _gocator->deregisterGrabCallback(_grabCallbackId);
        _grabCallbackId = 0;
    }
}
