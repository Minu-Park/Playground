#include "CameraSystem.h"
#include "Gocator.h"
#include "Utility/Qt/QCameraWidget.h"
#include "Utility/Qt/QGocatorWidget.h"
#include "Utility/Qt/QtConverter.h"
#include "engine/GraphicsEngine.h"
#include "BlazeScene3DAdapter.h"
#include "GocatorDataSetScene3DAdapter.h"
#include "Resources.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDockWidget>
#include <QFile>
#include <QIcon>
#include <QMainWindow>
#include <QMetaObject>
#include <QPointer>
#include <QTextStream>

#include <memory>
#include <utility>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    Resources::installResources(app);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle(QStringLiteral("Playground"));
    mainWindow.resize(1600, 900);

    auto* graphicsEngine = new GraphicsEngine(&mainWindow);
    mainWindow.setCentralWidget(graphicsEngine);

    auto cameraSystem = std::make_unique<CameraSystem>();
    Camera* camera = cameraSystem->addCamera();

    auto* cameraDock = new QDockWidget(QStringLiteral("Camera"), &mainWindow);
    auto* cameraWidget = new QCameraWidget(cameraDock, camera);
    cameraDock->setWidget(cameraWidget);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, cameraDock);

    auto gocator = std::make_unique<Gocator>();
    auto* gocatorDock = new QDockWidget(QStringLiteral("Gocator"), &mainWindow);
    auto* gocatorWidget = new QGocatorWidget(gocatorDock, gocator.get());
    gocatorDock->setWidget(gocatorWidget);
    mainWindow.addDockWidget(Qt::RightDockWidgetArea, gocatorDock);

    const auto grabCallbackId = camera->registerGrabCallback(
        [target = QPointer<GraphicsEngine>(graphicsEngine), camera](const CPylonImage& pylonImage, size_t) {
            QImage image = convertPylonImageToQImage(pylonImage);
            if (!image.isNull() && !target.isNull())
            {
                QMetaObject::invokeMethod(target.data(),
                                          [target, image = std::move(image)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setImage(image);
                                              }
                                          },
                                          Qt::QueuedConnection);
            }

            camera->ready();
        });

    const BlazeScene3DAdapter scene3DAdapter;
    const auto grab3DCallbackId = camera->registerGrab3DCallback(
        [target = QPointer<GraphicsEngine>(graphicsEngine), camera, scene3DAdapter](
            const Pylon::CPylonDataContainer& container,
            size_t) {
            if (target.isNull())
            {
                camera->ready();
                return;
            }

            const GraphicsScene3DRequest request = target->scene3DRequest();
            auto scene = scene3DAdapter.convert(container, request);
            if (scene.has_value())
            {
                QMetaObject::invokeMethod(target.data(),
                                          [target, scene = std::move(*scene)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setScene3D(std::move(scene));
                                              }
                                          },
                                          Qt::QueuedConnection);
            }

            camera->ready();
        });

    const auto gocatorGrabCallbackId = gocator->registerGrabCallback(
        [target = QPointer<GraphicsEngine>(graphicsEngine)](const GoPxLSdk::GoDataSet& dataSet, size_t) {
            GocatorDataSetScene3DAdapter adapter;

            GraphicsScene3DRequest request;
            request.content = GraphicsScene3DContent::RangeFrame;
            request.includeRangeAuxiliaryChannels = true;
            request.includePointCloudColors = true;
            request.retainSurfaceMesh = false;

            auto scene = adapter.convert(dataSet, request);
            if (scene.has_value() && !target.isNull())
            {
                QMetaObject::invokeMethod(target.data(),
                                          [target, scene = std::move(*scene)]() mutable {
                                              if (!target.isNull())
                                              {
                                                  target->setScene3D(std::move(scene));
                                              }
                                          },
                                          Qt::QueuedConnection);
            }
        });

    QObject::connect(&app,
                     &QCoreApplication::aboutToQuit,
                     [cameraWidget,
                      camera,
                      grabCallbackId,
                      grab3DCallbackId,
                      gocatorWidget,
                      gocator = gocator.get(),
                      gocatorGrabCallbackId]() {
                         cameraWidget->prepareForShutdown();
                         camera->deregisterGrabCallback(grabCallbackId);
                         camera->deregisterGrab3DCallback(grab3DCallbackId);
                         camera->stop();

                         gocatorWidget->prepareForShutdown();
                         gocator->deregisterGrabCallback(gocatorGrabCallbackId);
                         gocator->close();
                     });

    mainWindow.show();
    return app.exec();
}
