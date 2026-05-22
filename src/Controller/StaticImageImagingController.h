#pragma once
#include "Controller/AbstractImagingController.h"
#include <QStringList>
#include <QTimer>
#include <atomic>

class StaticImageImagingController : public AbstractImagingController {
    Q_OBJECT
public:
    explicit StaticImageImagingController(const QStringList& filePaths = QStringList(), QObject* parent = nullptr);
    ~StaticImageImagingController() override;

    // AbstractImagingController interface
    void start() override;
    void stop() override;
    bool isGrabbing() const override;
    void setSink(GraphicsEngineSink* sink) override;
    ProcessingPipeline* pipeline() override { return &_pipeline; }

    // Static image controller specific API
    void addImages(const QStringList& paths);
    void removeImage(int index);
    void setCurrentIndex(int index);
    int currentIndex() const;
    const QStringList& filePaths() const;
    
    void setFPS(int fps);
    int fps() const;

    void nextFrame();
    void prevFrame();

signals:
    void currentIndexChanged(int index);
    void filePathsChanged(const QStringList& paths);
    void grabbingStateChanged(bool isGrabbing);

private slots:
    void onTimerTimeout();

private:
    void loadAndProcessCurrent();

    QStringList _filePaths;
    int _currentIndex = 0;
    
    QTimer* _timer = nullptr;
    int _fps = 5; // Default to 5 FPS (200ms interval)
    bool _isGrabbing = false;

    GraphicsEngineSink* _sink = nullptr;
    ProcessingPipeline _pipeline;
    std::atomic<uint64_t> _frameSeq{0};
};
