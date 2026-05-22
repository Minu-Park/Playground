#include "Controller/GraphicsEngineSink.h"
#include "engine/GraphicsEngine.h"
#include <QMetaObject>

GraphicsEngineSink::GraphicsEngineSink(GraphicsEngine* engine, QObject* parent)
    : QObject(parent), _engine(engine) {}

void GraphicsEngineSink::enqueueImage(const QImage& image) {
    if (image.isNull()) return;
    QPointer<GraphicsEngine> target = _engine;
    QMetaObject::invokeMethod(this, [target, image]() {
        if (!target.isNull()) {
            target->setImage(image);
        }
    }, Qt::QueuedConnection);
}

void GraphicsEngineSink::enqueueScene3D(GraphicsScene3D&& scene) {
    QPointer<GraphicsEngine> target = _engine;
    QMetaObject::invokeMethod(this, [target, scene = std::move(scene)]() mutable {
        if (!target.isNull()) {
            target->setScene3D(std::move(scene));
        }
    }, Qt::QueuedConnection);
}

GraphicsEngine* GraphicsEngineSink::engine() const {
    return _engine.data();
}
