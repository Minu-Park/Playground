#pragma once
#include <QWidget>
#include <QPointer>

class QListWidget;
class QToolButton;
class QSpinBox;
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
    void updateButtonStates();

    QPointer<StaticImageImagingController> _controller;

    // UI elements
    QListWidget* _fileListWidget = nullptr;
    QToolButton* _addBtn = nullptr;
    QToolButton* _removeBtn = nullptr;
    QToolButton* _playPauseBtn = nullptr;
    QToolButton* _prevBtn = nullptr;
    QToolButton* _nextBtn = nullptr;
    QSpinBox* _fpsSpinBox = nullptr;
    QLabel* _fpsLabel = nullptr;
};
