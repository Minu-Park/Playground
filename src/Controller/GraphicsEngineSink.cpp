#include "Controller/GraphicsEngineSink.h"
#include "engine/GraphicsEngine.h"
#include <QMetaObject>

GraphicsEngineSink::GraphicsEngineSink(GraphicsEngine* engine, QObject* parent)
    : QObject(parent), _engine(engine) {}

void GraphicsEngineSink::enqueueImage(const QImage& image) {
    if (image.isNull()) return;
    QPointer<GraphicsEngine> target = _engine;
    QMetaObject::invokeMethod(this, [target, image]() {
        if (!target.isNull() && target->isVisible()) {
            target->setImage(image);
        }
    }, Qt::QueuedConnection);
}

void GraphicsEngineSink::enqueueScene3D(GraphicsScene3D&& scene) {
    QPointer<GraphicsEngine> target = _engine;
    QMetaObject::invokeMethod(this, [target, scene = std::move(scene)]() mutable {
        if (!target.isNull() && target->isVisible()) {
            target->setScene3D(std::move(scene));
        }
    }, Qt::QueuedConnection);
}

void GraphicsEngineSink::enqueueClear() {
    QPointer<GraphicsEngine> target = _engine;
    QMetaObject::invokeMethod(this, [target]() {
        if (!target.isNull() && target->isVisible()) {
            target->clearData();
        }
    }, Qt::QueuedConnection);
}

GraphicsEngine* GraphicsEngineSink::engine() const {
    return _engine.data();
}
