#include "StaticImageImagingController.h"
#include "GraphicsEngineSink.h"
#include <QImage>
#include <QDebug>

StaticImageImagingController::StaticImageImagingController(const QStringList& filePaths, QObject* parent)
    : AbstractImagingController(parent)
    , _filePaths(filePaths)
    , _timer(new QTimer(this)) {
    
    connect(_timer, &QTimer::timeout, this, &StaticImageImagingController::onTimerTimeout);
}

StaticImageImagingController::~StaticImageImagingController() {
    stop();
}

void StaticImageImagingController::start() {
    if (_filePaths.isEmpty()) {
        qWarning() << "[StaticImageImagingController] Cannot start: file list is empty.";
        return;
    }
    if (!_isGrabbing) {
        _isGrabbing = true;
        int interval = 1000 / _fps;
        _timer->start(interval);
        emit grabbingStateChanged(true);
        // Load the first frame immediately
        loadAndProcessCurrent();
    }
}

void StaticImageImagingController::stop() {
    if (_isGrabbing) {
        _timer->stop();
        _isGrabbing = false;
        emit grabbingStateChanged(false);
    }
}

bool StaticImageImagingController::isGrabbing() const {
    return _isGrabbing;
}

void StaticImageImagingController::setSink(GraphicsEngineSink* sink) {
    _sink = sink;
    // If we have images, push the current frame to the new sink
    if (_sink && !_filePaths.isEmpty()) {
        loadAndProcessCurrent();
    }
}

void StaticImageImagingController::addImages(const QStringList& paths) {
    if (paths.isEmpty()) return;
    
    _filePaths.append(paths);
    emit filePathsChanged(_filePaths);
    
    // If it was empty, select the first newly added image
    if (_currentIndex < 0 || _currentIndex >= _filePaths.size() - paths.size()) {
        _currentIndex = 0;
        emit currentIndexChanged(_currentIndex);
        loadAndProcessCurrent();
    }
}

void StaticImageImagingController::removeImage(int index) {
    if (index < 0 || index >= _filePaths.size()) return;
    
    _filePaths.removeAt(index);
    emit filePathsChanged(_filePaths);
    
    if (_filePaths.isEmpty()) {
        stop();
        _currentIndex = 0;
        emit currentIndexChanged(_currentIndex);
        // Clear screen with a null/empty image if possible
        if (_sink) {
            _sink->enqueueImage(QImage());
        }
    } else {
        if (_currentIndex >= _filePaths.size()) {
            _currentIndex = _filePaths.size() - 1;
            emit currentIndexChanged(_currentIndex);
        }
        loadAndProcessCurrent();
    }
}

void StaticImageImagingController::setCurrentIndex(int index) {
    if (index < 0 || index >= _filePaths.size()) return;
    
    if (_currentIndex != index) {
        _currentIndex = index;
        emit currentIndexChanged(_currentIndex);
        loadAndProcessCurrent();
    }
}

int StaticImageImagingController::currentIndex() const {
    return _currentIndex;
}

const QStringList& StaticImageImagingController::filePaths() const {
    return _filePaths;
}

void StaticImageImagingController::setFPS(int fps) {
    if (fps <= 0) return;
    _fps = fps;
    if (_isGrabbing) {
        int interval = 1000 / _fps;
        _timer->setInterval(interval);
    }
}

int StaticImageImagingController::fps() const {
    return _fps;
}

void StaticImageImagingController::nextFrame() {
    if (_filePaths.isEmpty()) return;
    int nextIdx = (_currentIndex + 1) % _filePaths.size();
    setCurrentIndex(nextIdx);
}

void StaticImageImagingController::prevFrame() {
    if (_filePaths.isEmpty()) return;
    int prevIdx = (_currentIndex - 1 + _filePaths.size()) % _filePaths.size();
    setCurrentIndex(prevIdx);
}

void StaticImageImagingController::onTimerTimeout() {
    if (_filePaths.isEmpty()) return;
    
    // Advance to next frame in automatic play mode
    _currentIndex = (_currentIndex + 1) % _filePaths.size();
    emit currentIndexChanged(_currentIndex);
    
    loadAndProcessCurrent();
}

void StaticImageImagingController::loadAndProcessCurrent() {
    if (!_sink || _filePaths.isEmpty()) return;
    if (_currentIndex < 0 || _currentIndex >= _filePaths.size()) return;
    
    QString path = _filePaths[_currentIndex];
    QImage rawImg(path);
    if (rawImg.isNull()) {
        qWarning() << "[StaticImageImagingController] Failed to load image:" << path;
        return;
    }
    
    ProcessingFrame frame;
    frame.payload = std::move(rawImg);
    frame.frameSeq = _frameSeq++;
    
    _pipeline.run(frame);
    
    if (std::holds_alternative<QImage>(frame.payload)) {
        _sink->enqueueImage(std::get<QImage>(frame.payload));
    }
}
