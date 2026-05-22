#include "UI/QStaticImageControlWidget.h"
#include "Controller/StaticImageImagingController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>

QStaticImageControlWidget::QStaticImageControlWidget(QWidget* parent)
    : QWidget(parent) {
    initUI();
}

void QStaticImageControlWidget::setController(StaticImageImagingController* controller) {
    if (_controller) {
        disconnect(_controller, nullptr, this, nullptr);
    }
    
    _controller = controller;
    
    if (_controller) {
        connect(_controller, &StaticImageImagingController::currentIndexChanged, this, &QStaticImageControlWidget::onCurrentIndexChanged);
        connect(_controller, &StaticImageImagingController::filePathsChanged, this, &QStaticImageControlWidget::onFilePathsChanged);
        connect(_controller, &StaticImageImagingController::grabbingStateChanged, this, &QStaticImageControlWidget::onGrabbingStateChanged);
        
        // Initialize UI values
        refreshList();
        onCurrentIndexChanged(_controller->currentIndex());
        onGrabbingStateChanged(_controller->isGrabbing());
        
        _fpsSlider->setValue(_controller->fps());
        _fpsLabel->setText(QStringLiteral("%1 FPS").arg(_controller->fps()));
        
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

void QStaticImageControlWidget::initUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Style properties
    QString btnStyle = QStringLiteral(
        "QPushButton {"
        "  background-color: #f0f4f8;"
        "  color: #16202b;"
        "  border: 1px solid #d9e1ea;"
        "  border-radius: 4px;"
        "  padding: 5px 10px;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #e2eaf4;"
        "}"
        "QPushButton:pressed {"
        "  background-color: #cbd8e9;"
        "}"
    );

    // List header
    auto* listHeader = new QLabel(QStringLiteral("Test Images List:"), this);
    listHeader->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #16202b;"));
    mainLayout->addWidget(listHeader);

    // File list
    _fileListWidget = new QListWidget(this);
    _fileListWidget->setStyleSheet(QStringLiteral(
        "QListWidget {"
        "  background-color: #ffffff;"
        "  border: 1px solid #d9e1ea;"
        "  border-radius: 6px;"
        "  padding: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #e2eaf4;"
        "  color: #007acc;"
        "  font-weight: bold;"
        "}"
    ));
    mainLayout->addWidget(_fileListWidget, 1); // Expand to fill space

    connect(_fileListWidget, &QListWidget::itemSelectionChanged, this, &QStaticImageControlWidget::handleListSelectionChanged);

    // Add/Remove buttons
    auto* listBtnLayout = new QHBoxLayout();
    _addBtn = new QPushButton(QStringLiteral("Add Image(s)"), this);
    _addBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #007acc;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #0062a3;"
        "}"
    ));
    
    _removeBtn = new QPushButton(QStringLiteral("Remove"), this);
    _removeBtn->setStyleSheet(btnStyle);

    listBtnLayout->addWidget(_addBtn);
    listBtnLayout->addWidget(_removeBtn);
    mainLayout->addLayout(listBtnLayout);

    connect(_addBtn, &QPushButton::clicked, this, &QStaticImageControlWidget::handleAddImages);
    connect(_removeBtn, &QPushButton::clicked, this, &QStaticImageControlWidget::handleRemoveSelectedImage);

    // Divider line
    auto* frameLine = new QFrame(this);
    frameLine->setFrameShape(QFrame::HLine);
    frameLine->setFrameShadow(QFrame::Sunken);
    frameLine->setStyleSheet(QStringLiteral("color: #d9e1ea;"));
    mainLayout->addWidget(frameLine);

    // Playback control header
    auto* playbackHeader = new QLabel(QStringLiteral("Playback Controls:"), this);
    playbackHeader->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 13px; color: #16202b;"));
    mainLayout->addWidget(playbackHeader);

    // Control buttons (Prev, Play/Pause, Next)
    auto* ctrlLayout = new QHBoxLayout();
    _prevBtn = new QPushButton(QStringLiteral("◀ Prev"), this);
    _prevBtn->setStyleSheet(btnStyle);
    
    _playPauseBtn = new QPushButton(QStringLiteral("▶ Play"), this);
    _playPauseBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background-color: #28a745;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 16px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #218838;"
        "}"
    ));

    _nextBtn = new QPushButton(QStringLiteral("Next ▶"), this);
    _nextBtn->setStyleSheet(btnStyle);

    ctrlLayout->addWidget(_prevBtn);
    ctrlLayout->addWidget(_playPauseBtn);
    ctrlLayout->addWidget(_nextBtn);
    mainLayout->addLayout(ctrlLayout);

    connect(_prevBtn, &QPushButton::clicked, this, &QStaticImageControlWidget::handlePrev);
    connect(_playPauseBtn, &QPushButton::clicked, this, &QStaticImageControlWidget::handlePlayPause);
    connect(_nextBtn, &QPushButton::clicked, this, &QStaticImageControlWidget::handleNext);

    // FPS Slider
    auto* fpsHeaderLayout = new QHBoxLayout();
    auto* fpsLabelTitle = new QLabel(QStringLiteral("Playback Speed:"), this);
    _fpsLabel = new QLabel(QStringLiteral("5 FPS"), this);
    _fpsLabel->setAlignment(Qt::AlignRight);
    
    fpsHeaderLayout->addWidget(fpsLabelTitle);
    fpsHeaderLayout->addWidget(_fpsLabel);
    mainLayout->addLayout(fpsHeaderLayout);

    _fpsSlider = new QSlider(Qt::Horizontal, this);
    _fpsSlider->setRange(1, 30);
    _fpsSlider->setValue(5);
    _fpsSlider->setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal {"
        "  border: 1px solid #d9e1ea;"
        "  height: 6px;"
        "  background: #f0f4f8;"
        "  border-radius: 3px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #007acc;"
        "  border: none;"
        "  width: 14px;"
        "  height: 14px;"
        "  margin: -4px 0;"
        "  border-radius: 7px;"
        "}"
    ));
    mainLayout->addWidget(_fpsSlider);

    connect(_fpsSlider, &QSlider::valueChanged, this, &QStaticImageControlWidget::handleFpsChanged);
    
    setEnabled(false); // Enable only when a controller is set
}

void QStaticImageControlWidget::refreshList() {
    if (!_controller) return;
    
    // Temporarily block list signals to prevent recursion
    _fileListWidget->blockSignals(true);
    _fileListWidget->clear();
    
    const auto& paths = _controller->filePaths();
    for (const auto& path : paths) {
        QFileInfo info(path);
        _fileListWidget->addItem(info.fileName());
    }
    _fileListWidget->blockSignals(false);
}

void QStaticImageControlWidget::handleAddImages() {
    if (!_controller) return;
    
    QStringList files = QFileDialog::getOpenFileNames(
        this,
        QStringLiteral("Add Test Images"),
        QString(),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.bmp *.pgm *.tif);;All Files (*)")
    );
    
    if (!files.isEmpty()) {
        _controller->addImages(files);
    }
}

void QStaticImageControlWidget::handleRemoveSelectedImage() {
    if (!_controller) return;
    int index = _fileListWidget->currentRow();
    if (index >= 0) {
        _controller->removeImage(index);
    }
}

void QStaticImageControlWidget::handlePlayPause() {
    if (!_controller) return;
    
    if (_controller->isGrabbing()) {
        _controller->stop();
    } else {
        _controller->start();
    }
}

void QStaticImageControlWidget::handlePrev() {
    if (_controller) {
        _controller->prevFrame();
    }
}

void QStaticImageControlWidget::handleNext() {
    if (_controller) {
        _controller->nextFrame();
    }
}

void QStaticImageControlWidget::handleFpsChanged(int value) {
    if (_controller) {
        _controller->setFPS(value);
        _fpsLabel->setText(QStringLiteral("%1 FPS").arg(value));
    }
}

void QStaticImageControlWidget::handleListSelectionChanged() {
    if (!_controller) return;
    int index = _fileListWidget->currentRow();
    if (index >= 0 && index < _fileListWidget->count()) {
        _controller->setCurrentIndex(index);
    }
}

void QStaticImageControlWidget::onCurrentIndexChanged(int index) {
    if (index >= 0 && index < _fileListWidget->count()) {
        _fileListWidget->blockSignals(true);
        _fileListWidget->setCurrentRow(index);
        _fileListWidget->blockSignals(false);
    }
}

void QStaticImageControlWidget::onFilePathsChanged(const QStringList& paths) {
    Q_UNUSED(paths);
    refreshList();
    if (_controller) {
        onCurrentIndexChanged(_controller->currentIndex());
    }
}

void QStaticImageControlWidget::onGrabbingStateChanged(bool isGrabbing) {
    if (isGrabbing) {
        _playPauseBtn->setText(QStringLiteral("⏸ Pause"));
        _playPauseBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #dc3545;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 6px 16px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #bd2130;"
            "}"
        ));
    } else {
        _playPauseBtn->setText(QStringLiteral("▶ Play"));
        _playPauseBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: #28a745;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 6px 16px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #218838;"
            "}"
        ));
    }
}
