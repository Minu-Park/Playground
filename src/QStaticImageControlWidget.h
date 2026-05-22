#pragma once
#include <QWidget>
#include <QPointer>

class QListWidget;
class QPushButton;
class QSlider;
class QLabel;
class StaticImageImagingController;

class QStaticImageControlWidget : public QWidget {
    Q_OBJECT
public:
    explicit QStaticImageControlWidget(QWidget* parent = nullptr);
    ~QStaticImageControlWidget() override = default;

    void setController(StaticImageImagingController* controller);

private slots:
    void handleAddImages();
    void handleRemoveSelectedImage();
    void handlePlayPause();
    void handlePrev();
    void handleNext();
    void handleFpsChanged(int value);
    void handleListSelectionChanged();

    // Controller sync slots
    void onCurrentIndexChanged(int index);
    void onFilePathsChanged(const QStringList& paths);
    void onGrabbingStateChanged(bool isGrabbing);

private:
    void initUI();
    void refreshList();

    QPointer<StaticImageImagingController> _controller;

    // UI elements
    QListWidget* _fileListWidget = nullptr;
    QPushButton* _addBtn = nullptr;
    QPushButton* _removeBtn = nullptr;
    QPushButton* _playPauseBtn = nullptr;
    QPushButton* _prevBtn = nullptr;
    QPushButton* _nextBtn = nullptr;
    QSlider* _fpsSlider = nullptr;
    QLabel* _fpsLabel = nullptr;
};
