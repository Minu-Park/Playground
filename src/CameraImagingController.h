#pragma once
#include "AbstractImagingController.h"
#include "Camera.h"
#include <atomic>
#include <mutex>

class GraphicsEngineSink;

#ifdef HAS_OPENCV
#include <opencv2/opencv.hpp>
using ProcessFunc = void (*)(const cv::Mat&, cv::Mat&, double, double);
#else
using ProcessFunc = void*;
#endif

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

#ifdef HAS_OPENCV
    void setFilter(ProcessFunc filter);
    void setParameters(double p1, double p2);
#endif

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

#ifdef HAS_OPENCV
    std::mutex _filterMutex;
    ProcessFunc _activeFilter = nullptr;
    double _param1 = 5.0;
    double _param2 = 50.0;
#endif
};
