#pragma once
#include <QObject>
#include "Pipeline/ProcessingPipeline.h"

class GraphicsEngineSink;

class AbstractImagingController : public QObject {
    Q_OBJECT
public:
    explicit AbstractImagingController(QObject* parent = nullptr) : QObject(parent) {}
    ~AbstractImagingController() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool isGrabbing() const = 0;
    virtual void setSink(GraphicsEngineSink* sink) = 0;
    virtual ProcessingPipeline* pipeline() = 0;
};
