#include "UI/QStaticImageControlWidget.h"
#include "Controller/StaticImageImagingController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QToolButton>
#include <QSpinBox>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QStyle>
#include <QDebug>

QStaticImageControlWidget::QStaticImageControlWidget(QWidget* parent)
    : QWidget(parent) {
    setMinimumSize(280, 220);
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
        
        _fpsSpinBox->setValue(_controller->fps());
        if (_fpsLabel) {
            _fpsLabel->setText(QStringLiteral("%1 FPS").arg(_controller->fps()));
        }
        
        setEnabled(true);
        updateButtonStates();
    } else {
        setEnabled(false);
    }
}

void QStaticImageControlWidget::initUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Create the top control layout (horizontal bar)
    auto* topControlLayout = new QHBoxLayout();
    topControlLayout->setContentsMargins(12, 12, 12, 12);
    topControlLayout->setSpacing(10);

    // 1. Playback speed controls (FPS SpinBox)
    auto* fpsLayout = new QHBoxLayout();
    fpsLayout->setSpacing(6);
    auto* fpsLabelTitle = new QLabel(QStringLiteral("FPS:"), this);
    _fpsSpinBox = new QSpinBox(this);
    _fpsSpinBox->setObjectName(QStringLiteral("StaticImageFpsSpinBox"));
    _fpsSpinBox->setRange(1, 30);
    _fpsSpinBox->setValue(5);
    _fpsSpinBox->setFixedWidth(60);
    fpsLayout->addWidget(fpsLabelTitle);
    fpsLayout->addWidget(_fpsSpinBox);

    // 2. Playback buttons (Prev, Play/Pause, Next)
    auto* playbackLayout = new QHBoxLayout();
    playbackLayout->setSpacing(4);

    _prevBtn = new QToolButton(this);
    _prevBtn->setObjectName(QStringLiteral("StaticImagePrevBtn"));
    _prevBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _prevBtn->setIconSize(QSize(16, 16));
    _prevBtn->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-back-48.png")));

    _playPauseBtn = new QToolButton(this);
    _playPauseBtn->setObjectName(QStringLiteral("StaticImagePlayPauseBtn"));
    _playPauseBtn->setCheckable(true);
    _playPauseBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _playPauseBtn->setIconSize(QSize(16, 16));
    {
        QIcon icon;
        icon.addFile(QStringLiteral(":/Resources/Icons/icons8-play-48.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(QStringLiteral(":/Resources/Icons/icons8-pause-48.png"), QSize(), QIcon::Normal, QIcon::On);
        _playPauseBtn->setIcon(icon);
    }

    _nextBtn = new QToolButton(this);
    _nextBtn->setObjectName(QStringLiteral("StaticImageNextBtn"));
    _nextBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _nextBtn->setIconSize(QSize(16, 16));
    _nextBtn->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-forward-48.png")));

    playbackLayout->addWidget(_prevBtn);
    playbackLayout->addWidget(_playPauseBtn);
    playbackLayout->addWidget(_nextBtn);

    // 3. List actions (Add, Remove)
    auto* listActionLayout = new QHBoxLayout();
    listActionLayout->setSpacing(4);

    _addBtn = new QToolButton(this);
    _addBtn->setObjectName(QStringLiteral("StaticImageAddBtn"));
    _addBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _addBtn->setIconSize(QSize(16, 16));
    _addBtn->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-add-image-48.png")));

    _removeBtn = new QToolButton(this);
    _removeBtn->setObjectName(QStringLiteral("StaticImageRemoveBtn"));
    _removeBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _removeBtn->setIconSize(QSize(16, 16));
    _removeBtn->setIcon(QIcon(QStringLiteral(":/Resources/Icons/icons8-delete-48.png")));

    listActionLayout->addWidget(_addBtn);
    listActionLayout->addWidget(_removeBtn);

    // Assemble top bar
    topControlLayout->addLayout(fpsLayout);
    topControlLayout->addLayout(playbackLayout);
    topControlLayout->addLayout(listActionLayout);
    topControlLayout->addStretch(1);

    mainLayout->addLayout(topControlLayout);

    // 4. File List Widget Layout
    auto* listLayout = new QVBoxLayout();
    listLayout->setContentsMargins(12, 0, 12, 12);
    listLayout->setSpacing(8);

    _fileListWidget = new QListWidget(this);
    _fileListWidget->setObjectName(QStringLiteral("StaticImageFileListWidget"));
    listLayout->addWidget(_fileListWidget);

    mainLayout->addLayout(listLayout);

    // Connections
    connect(_fpsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &QStaticImageControlWidget::handleFpsChanged);
    connect(_prevBtn, &QToolButton::clicked, this, &QStaticImageControlWidget::handlePrev);
    connect(_playPauseBtn, &QToolButton::clicked, this, &QStaticImageControlWidget::handlePlayPause);
    connect(_nextBtn, &QToolButton::clicked, this, &QStaticImageControlWidget::handleNext);
    connect(_addBtn, &QToolButton::clicked, this, &QStaticImageControlWidget::handleAddImages);
    connect(_removeBtn, &QToolButton::clicked, this, &QStaticImageControlWidget::handleRemoveSelectedImage);
    connect(_fileListWidget, &QListWidget::itemSelectionChanged, this, &QStaticImageControlWidget::handleListSelectionChanged);

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
    updateButtonStates();
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
        if (_fpsLabel) {
            _fpsLabel->setText(QStringLiteral("%1 FPS").arg(value));
        }
    }
}

void QStaticImageControlWidget::handleListSelectionChanged() {
    if (!_controller) return;
    int index = _fileListWidget->currentRow();
    if (index >= 0 && index < _fileListWidget->count()) {
        _controller->setCurrentIndex(index);
    }
    updateButtonStates();
}

void QStaticImageControlWidget::onCurrentIndexChanged(int index) {
    if (index >= 0 && index < _fileListWidget->count()) {
        _fileListWidget->blockSignals(true);
        _fileListWidget->setCurrentRow(index);
        _fileListWidget->blockSignals(false);
    }
    updateButtonStates();
}

void QStaticImageControlWidget::onFilePathsChanged(const QStringList& paths) {
    Q_UNUSED(paths);
    refreshList();
    if (_controller) {
        onCurrentIndexChanged(_controller->currentIndex());
    }
    updateButtonStates();
}

void QStaticImageControlWidget::onGrabbingStateChanged(bool isGrabbing) {
    QSignalBlocker block(_playPauseBtn);
    _playPauseBtn->setChecked(isGrabbing);
}

void QStaticImageControlWidget::updateButtonStates() {
    bool hasImages = _controller && !_controller->filePaths().isEmpty();
    _playPauseBtn->setEnabled(hasImages);
    _prevBtn->setEnabled(hasImages);
    _nextBtn->setEnabled(hasImages);
    _removeBtn->setEnabled(hasImages && _fileListWidget->currentRow() >= 0);
}
