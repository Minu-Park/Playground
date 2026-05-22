#pragma once
#include "Controller/AbstractImagingController.h"
#include "Gocator.h"
#include <atomic>

class GraphicsEngineSink;

class GocatorImagingController : public AbstractImagingController {
    Q_OBJECT
public:
    explicit GocatorImagingController(Gocator* gocator, QObject* parent = nullptr);
    ~GocatorImagingController() override;

    void start() override;
    void stop() override;
    bool isGrabbing() const override;
    void setSink(GraphicsEngineSink* sink) override;
    ProcessingPipeline* pipeline() override { return &_pipeline; }

private:
    void registerCallbacks();
    void deregisterCallbacks();

    Gocator* _gocator;
    GraphicsEngineSink* _sink = nullptr;
    std::atomic<bool> _isGrabbing{false};
    ProcessingPipeline _pipeline;
    std::atomic<uint64_t> _frameSeq{0};

    Gocator::CallbackId _grabCallbackId = 0;
    Gocator::CallbackId _statusCallbackId = 0;
};
