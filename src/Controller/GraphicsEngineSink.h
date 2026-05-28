#pragma once
#include <QObject>
#include <QPointer>
#include <QImage>
#include "engine/GraphicsSceneTypes.h"

class GraphicsEngine;

class GraphicsEngineSink : public QObject {
    Q_OBJECT
public:
    explicit GraphicsEngineSink(GraphicsEngine* engine, QObject* parent = nullptr);
    ~GraphicsEngineSink() override = default;

    void enqueueImage(const QImage& image);
    void enqueueScene3D(GraphicsScene3D&& scene);
    void enqueueClear();

    GraphicsEngine* engine() const;

private:
    QPointer<GraphicsEngine> _engine;
};
