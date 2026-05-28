#pragma once
#include "Controller/AbstractImagingController.h"
#include "Camera.h"
#include <atomic>

class GraphicsEngineSink;

class CameraImagingController : public AbstractImagingController {
    Q_OBJECT
public:
    explicit CameraImagingController(Camera* camera, QObject* parent = nullptr);
    ~CameraImagingController() override;

    void start() override;
    void stop() override;
    bool isGrabbing() const override;
    void setSink(GraphicsEngineSink* sink) override;
    ProcessingPipeline* pipeline() override { return &_pipeline; }

private:
    void registerCallbacks();
    void deregisterCallbacks();

    Camera* _camera;
    GraphicsEngineSink* _sink = nullptr;
    std::atomic<bool> _isGrabbing{false};
    ProcessingPipeline _pipeline;
    std::atomic<uint64_t> _frameSeq{0};

    Camera::CallbackId _grabCallbackId = 0;
    Camera::CallbackId _grab3DCallbackId = 0;
    Camera::CallbackId _statusCallbackId = 0;
};
