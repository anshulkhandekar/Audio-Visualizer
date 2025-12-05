#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QFormLayout>
#include <QMouseEvent>
#include <QEvent>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), audioPlayer(nullptr),
      maxMagnitude(0.0f), maxMagnitudeInitialized(false),
      isDragging(false), dragStartBin(-1), dragEndBin(-1), activeDragView(nullptr),
      isUserScrubbing(false) {
    setupUI();
    
    // Create audio player
    audioPlayer = new AudioPlayer(this);
    
    // Connect signals - use QueuedConnection to avoid blocking the audio callback thread
    connect(audioPlayer, &AudioPlayer::fftDataReady, this, &MainWindow::onFFTDataReady, Qt::QueuedConnection);
    connect(audioPlayer, &AudioPlayer::playbackFinished, this, &MainWindow::onPlaybackFinished, Qt::QueuedConnection);
    connect(audioPlayer, &AudioPlayer::positionChanged, this, &MainWindow::onPositionChanged, Qt::QueuedConnection);
    
    // Connect position slider (after audioPlayer is created)
    connect(positionSlider, &QSlider::sliderPressed, this, [this]() {
        isUserScrubbing = true;
        // Pause playback while scrubbing for better control
        if (audioPlayer && audioPlayer->isPlaying() && !audioPlayer->isPaused()) {
            audioPlayer->pausePlayback();
            pauseButton->setEnabled(false);
            playButton->setEnabled(true);
        }
    });
    connect(positionSlider, &QSlider::sliderReleased, this, &MainWindow::onPositionSliderReleased);
    connect(positionSlider, &QSlider::valueChanged, this, &MainWindow::onPositionSliderChanged);
    
    // Setup position update timer
    positionUpdateTimer = new QTimer(this);
    connect(positionUpdateTimer, &QTimer::timeout, this, &MainWindow::updatePositionDisplay);
    positionUpdateTimer->start(100); // Update every 100ms
    
    // Setup FFT update timer for throttling chart updates (30 FPS = ~33ms)
    // This prevents signal queue buildup when FFTs are computed faster than charts can update
    fftUpdateTimer = new QTimer(this);
    connect(fftUpdateTimer, &QTimer::timeout, this, &MainWindow::onFFTUpdateTimer);
    fftUpdateTimer->start(33); // Update charts at ~30 FPS
    
    // Connect buttons
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::onVolumeSliderChanged);
    
    // Connect filter controls
    connect(lowPassSlider, &QSlider::valueChanged, this, &MainWindow::onLowPassSliderChanged);
    connect(highPassSlider, &QSlider::valueChanged, this, &MainWindow::onHighPassSliderChanged);
    connect(bandStartSlider, &QSlider::valueChanged, this, &MainWindow::onBandStartSliderChanged);
    connect(bandEndSlider, &QSlider::valueChanged, this, &MainWindow::onBandEndSliderChanged);
    connect(lowPassCheckbox, &QCheckBox::stateChanged, this, &MainWindow::onLowPassCheckboxChanged);
    connect(highPassCheckbox, &QCheckBox::stateChanged, this, &MainWindow::onHighPassCheckboxChanged);
    connect(bandStopCheckbox, &QCheckBox::stateChanged, this, &MainWindow::onBandStopCheckboxChanged);
    
    // Install event filters for click-and-drag
    histogramView->installEventFilter(this);
    linePlotView->installEventFilter(this);
    // Note: Radial view uses custom widget, may need separate event handling
    
    setPlaybackControlsEnabled(false);
    statusLabel->setText("No file loaded");
}

MainWindow::~MainWindow() {
    if (audioPlayer) {
        audioPlayer->stopPlayback();
    }
}

void MainWindow::setupUI() {
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    mainLayout = new QVBoxLayout(centralWidget);
    
    // Status label
    statusLabel = new QLabel("Ready", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel);
    
    // Button layout
    buttonLayout = new QHBoxLayout();
    
    loadButton = new QPushButton("Load File", this);
    playButton = new QPushButton("Play", this);
    pauseButton = new QPushButton("Pause", this);
    stopButton = new QPushButton("Stop", this);
    exportButton = new QPushButton("Export Audio", this);
    
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(exportButton);
    
    // Volume control
    QLabel* volumeTextLabel = new QLabel("Volume:", this);
    volumeSlider = new QSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(100); // Default to 100%
    volumeSlider->setMaximumWidth(150);
    volumeLabel = new QLabel("100%", this);
    volumeLabel->setMinimumWidth(50);
    volumeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    buttonLayout->addWidget(volumeTextLabel);
    buttonLayout->addWidget(volumeSlider);
    buttonLayout->addWidget(volumeLabel);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Position scrubber
    QHBoxLayout* positionLayout = new QHBoxLayout();
    positionLabel = new QLabel("00:00", this);
    positionLabel->setMinimumWidth(60);
    positionLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    
    positionSlider = new QSlider(Qt::Horizontal, this);
    positionSlider->setRange(0, 1000);
    positionSlider->setValue(0);
    positionSlider->setEnabled(false);
    
    durationLabel = new QLabel("00:00", this);
    durationLabel->setMinimumWidth(60);
    durationLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    positionLayout->addWidget(positionLabel);
    positionLayout->addWidget(positionSlider);
    positionLayout->addWidget(durationLabel);
    
    mainLayout->addLayout(positionLayout);
    
    // Filter controls
    filterGroup = new QGroupBox("Frequency Filters", this);
    QGridLayout* filterLayout = new QGridLayout(filterGroup);
    
    // Low-pass filter
    lowPassCheckbox = new QCheckBox("Enable", this);
    lowPassSlider = new QSlider(Qt::Horizontal, this);
    lowPassSlider->setRange(0, 5000);
    lowPassSlider->setValue(0);
    lowPassLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("Low-Pass (cut above):", this), 0, 0);
    filterLayout->addWidget(lowPassCheckbox, 0, 1);
    filterLayout->addWidget(lowPassSlider, 0, 2);
    filterLayout->addWidget(lowPassLabel, 0, 3);
    
    // High-pass filter
    highPassCheckbox = new QCheckBox("Enable", this);
    highPassSlider = new QSlider(Qt::Horizontal, this);
    highPassSlider->setRange(0, 5000);
    highPassSlider->setValue(0);
    highPassLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("High-Pass (cut below):", this), 1, 0);
    filterLayout->addWidget(highPassCheckbox, 1, 1);
    filterLayout->addWidget(highPassSlider, 1, 2);
    filterLayout->addWidget(highPassLabel, 1, 3);
    
    // Band-stop filter
    bandStopCheckbox = new QCheckBox("Enable", this);
    bandStartSlider = new QSlider(Qt::Horizontal, this);
    bandStartSlider->setRange(0, 5000);
    bandStartSlider->setValue(0);
    bandStartLabel = new QLabel("0 Hz", this);
    bandEndSlider = new QSlider(Qt::Horizontal, this);
    bandEndSlider->setRange(0, 5000);
    bandEndSlider->setValue(0);
    bandEndLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("Band-Stop (cut range):", this), 2, 0);
    // Combine checkbox and "Start:" label in column 1 using a horizontal layout
    QWidget* checkboxStartWidget = new QWidget(this);
    QHBoxLayout* checkboxStartLayout = new QHBoxLayout(checkboxStartWidget);
    checkboxStartLayout->setContentsMargins(0, 0, 0, 0);
    checkboxStartLayout->setSpacing(5);
    checkboxStartLayout->addWidget(bandStopCheckbox);
    QLabel* startLabel = new QLabel("Start:", this);
    startLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    checkboxStartLayout->addWidget(startLabel);
    checkboxStartLayout->addStretch();
    filterLayout->addWidget(checkboxStartWidget, 2, 1);
    // Start slider in column 2 to align with other filter sliders
    filterLayout->addWidget(bandStartSlider, 2, 2);
    filterLayout->addWidget(bandStartLabel, 2, 3);
    // End controls
    QLabel* endLabel = new QLabel("End:", this);
    endLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    filterLayout->addWidget(endLabel, 2, 4);
    filterLayout->addWidget(bandEndSlider, 2, 5);
    filterLayout->addWidget(bandEndLabel, 2, 6);
    
    // Set column stretch to align sliders properly with other filters
    filterLayout->setColumnStretch(0, 0);  // Label column
    filterLayout->setColumnStretch(1, 0);  // Checkbox + Start label column
    filterLayout->setColumnStretch(2, 1);  // Start slider column (aligns with other filters)
    filterLayout->setColumnStretch(3, 0);  // Start value label
    filterLayout->setColumnStretch(4, 0);  // End label
    filterLayout->setColumnStretch(5, 1);  // End slider column
    filterLayout->setColumnStretch(6, 0);  // End value label
    
    mainLayout->addWidget(filterGroup);
    
    // Tab widget for visualizations
    tabWidget = new QTabWidget(this);
    
    // Histogram visualization
    histogramChart = new QChart();
    histogramChart->setTitle("Frequency Spectrum - Histogram");
    histogramChart->setAnimationOptions(QChart::NoAnimation);
    
    barSeries = new QBarSeries();
    barSet = new QBarSet("Magnitude");
    
    QStringList categories;
    for (int i = 0; i < MAX_BARS; i++) {
        *barSet << 0.0;
        categories << QString::number(i);
    }
    
    barSeries->append(barSet);
    histogramChart->addSeries(barSeries);
    
    histogramAxisX = new QBarCategoryAxis();
    histogramAxisX->append(categories);
    histogramAxisX->setTitleText("Frequency Bin");
    histogramChart->addAxis(histogramAxisX, Qt::AlignBottom);
    barSeries->attachAxis(histogramAxisX);
    
    histogramAxisY = new QValueAxis();
    histogramAxisY->setTitleText("Magnitude");
    histogramAxisY->setRange(0, 1000);
    histogramChart->addAxis(histogramAxisY, Qt::AlignLeft);
    barSeries->attachAxis(histogramAxisY);
    
    histogramView = new QChartView(histogramChart, this);
    histogramView->setRenderHint(QPainter::Antialiasing);
    histogramView->setMinimumHeight(400);
    tabWidget->addTab(histogramView, "Histogram");
    
    // Line Plot visualization
    linePlotChart = new QChart();
    linePlotChart->setTitle("Frequency Spectrum - Line Plot");
    linePlotChart->setAnimationOptions(QChart::NoAnimation);
    
    lineSeries = new QLineSeries();
    lineSeries->setName("Magnitude");
    
    linePlotChart->addSeries(lineSeries);
    
    linePlotAxisX = new QValueAxis();
    linePlotAxisX->setTitleText("Frequency Bin");
    linePlotAxisX->setRange(0, MAX_BARS);
    linePlotChart->addAxis(linePlotAxisX, Qt::AlignBottom);
    lineSeries->attachAxis(linePlotAxisX);
    
    linePlotAxisY = new QValueAxis();
    linePlotAxisY->setTitleText("Magnitude");
    linePlotAxisY->setRange(0, 1000);
    linePlotChart->addAxis(linePlotAxisY, Qt::AlignLeft);
    lineSeries->attachAxis(linePlotAxisY);
    
    linePlotView = new QChartView(linePlotChart, this);
    linePlotView->setRenderHint(QPainter::Antialiasing);
    linePlotView->setMinimumHeight(400);
    tabWidget->addTab(linePlotView, "Line Plot");
    
    // Radial visualization (custom polar plot)
    radialView = new RadialVisualizationWidget(this);
    radialView->setMinimumHeight(400);
    tabWidget->addTab(radialView, "Radial");
    
    mainLayout->addWidget(tabWidget);
    
    // Window settings
    setWindowTitle("Audio Visualizer");
    resize(1000, 700);
}

void MainWindow::onLoadFileClicked() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open Audio File",
        "",
        "Audio Files (*.mp3 *.wav);;MP3 Files (*.mp3);;WAV Files (*.wav);;All Files (*.*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    statusLabel->setText("Loading file...");
    QApplication::processEvents(); // Update UI
    
    if (audioPlayer->loadFile(filename.toStdString())) {
        statusLabel->setText("File loaded: " + QFileInfo(filename).fileName());
        setPlaybackControlsEnabled(true);
        
        // Update position slider range
        size_t totalLength = audioPlayer->getTotalLength();
        positionSlider->setRange(0, (int)totalLength);
        positionSlider->setValue(0);
        positionSlider->setEnabled(true);
        
        // Update duration label
        updatePositionDisplay();
        
        // Clear FFT data
        {
            QMutexLocker locker(&fftDataMutex);
            latestFFTData.clear();
        }
        
        // Reset visualization state for new file
        // Clear charts
        for (int i = 0; i < MAX_BARS; i++) {
            barSet->replace(i, 0.0);
        }
        lineSeries->clear();
        radialView->updateData(std::vector<float>());
        
        // Reset smoothing state
        magnitudeHistory.clear();
        previousSmoothed.clear();
        maxMagnitude = 0.0f;
        maxMagnitudeInitialized = false;
        
        // Reset axis ranges to default
        histogramAxisY->setRange(0, 1000);
        linePlotAxisY->setRange(0, 1000);
    } else {
        QMessageBox::critical(this, "Error", "Failed to load audio file: " + filename);
        statusLabel->setText("Failed to load file");
        setPlaybackControlsEnabled(false);
        positionSlider->setEnabled(false);
    }
}

void MainWindow::onPlayClicked() {
    if (audioPlayer->startPlayback()) {
        statusLabel->setText("Playing...");
        playButton->setEnabled(false);
        pauseButton->setEnabled(true);
        stopButton->setEnabled(true);
        positionUpdateTimer->start(100); // Start position updates
    } else {
        QMessageBox::critical(this, "Error", "Failed to start playback");
    }
}

void MainWindow::onPauseClicked() {
    if (audioPlayer->isPlaying() && !audioPlayer->isPaused()) {
        audioPlayer->pausePlayback();
        statusLabel->setText("Paused");
        playButton->setEnabled(true);
        pauseButton->setEnabled(false);
    } else if (audioPlayer->isPaused()) {
        audioPlayer->resumePlayback();
        statusLabel->setText("Playing...");
        playButton->setEnabled(false);
        pauseButton->setEnabled(true);
    }
}

void MainWindow::onStopClicked() {
    audioPlayer->stopPlayback();
    statusLabel->setText("Stopped");
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    
    // Reset position slider
    positionSlider->setValue(0);
    updatePositionDisplay();
    
    // Clear FFT data
    {
        QMutexLocker locker(&fftDataMutex);
        latestFFTData.clear();
    }
    
    // Reset charts
    for (int i = 0; i < MAX_BARS; i++) {
        barSet->replace(i, 0.0);
    }
    lineSeries->clear();
    radialView->updateData(std::vector<float>());
    
    // Reset smoothing state
    magnitudeHistory.clear();
    previousSmoothed.clear();
    maxMagnitude = 0.0f;
    maxMagnitudeInitialized = false;
}

void MainWindow::onExportClicked() {
    if (!audioPlayer || audioPlayer->getSampleRate() == 0 || audioPlayer->getTotalLength() == 0) {
        QMessageBox::warning(this, "Export Edited Audio", "No audio loaded to export.");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Edited Audio (WAV)",
        "",
        "WAV Files (*.wav)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    statusLabel->setText("Exporting edited audio...");
    QApplication::processEvents();
    setPlaybackControlsEnabled(false);

    bool ok = audioPlayer->exportEditedToWav(filePath.toStdString());

    setPlaybackControlsEnabled(true);

    if (ok) {
        statusLabel->setText("Export complete: " + QFileInfo(filePath).fileName());
        QMessageBox::information(this, "Export Edited Audio",
                                 "Successfully exported edited audio to:\n" + filePath);
    } else {
        statusLabel->setText("Export failed");
        QMessageBox::critical(this, "Export Edited Audio",
                              "Failed to export edited audio. Please check the path and try again.");
    }
}

void MainWindow::onFFTDataReady(const std::vector<float>& magnitudes) {
    // Store the latest FFT data (thread-safe)
    // This prevents signal queue buildup - we update charts at a fixed rate via timer
    QMutexLocker locker(&fftDataMutex);
    latestFFTData = magnitudes;
}

void MainWindow::onFFTUpdateTimer() {
    // Get latest FFT data (thread-safe)
    std::vector<float> magnitudes;
    {
        QMutexLocker locker(&fftDataMutex);
        if (latestFFTData.empty()) {
            return; // No data yet
        }
        magnitudes = latestFFTData;
    }
    
    // Apply smoothing
    std::vector<float> smoothed = smoothMagnitudes(magnitudes);
    
    // Update all visualizations
    updateChart(smoothed);
}

void MainWindow::onPlaybackFinished() {
    statusLabel->setText("Playback finished");
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    
    // Update position to end
    updatePositionDisplay();
    
    // Clear FFT data
    {
        QMutexLocker locker(&fftDataMutex);
        latestFFTData.clear();
    }
    
    // Reset charts
    for (int i = 0; i < MAX_BARS; i++) {
        barSet->replace(i, 0.0);
    }
    lineSeries->clear();
    radialView->updateData(std::vector<float>());
    
    // Reset smoothing state
    magnitudeHistory.clear();
    previousSmoothed.clear();
    maxMagnitude = 0.0f;
    maxMagnitudeInitialized = false;
}

void MainWindow::updateChart(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return;
    }
    
    // Only update the currently visible chart to reduce CPU usage
    // This prevents expensive updates to hidden charts from causing slowdowns
    int currentTab = tabWidget->currentIndex();
    
    if (currentTab == 0) {
        // Histogram tab is visible
        updateHistogram(magnitudes);
    } else if (currentTab == 1) {
        // Line plot tab is visible
        updateLinePlot(magnitudes);
    } else if (currentTab == 2) {
        // Radial tab is visible
        updateRadial(magnitudes);
    }
}

std::vector<float> MainWindow::smoothMagnitudes(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return magnitudes;
    }
    
    std::vector<float> smoothed(magnitudes.size());
    
    // Apply EMA
    if (previousSmoothed.empty()) {
        previousSmoothed = magnitudes;
        smoothed = magnitudes;
    } else {
        for (size_t i = 0; i < magnitudes.size(); i++) {
            smoothed[i] = EMA_ALPHA * magnitudes[i] + (1.0f - EMA_ALPHA) * previousSmoothed[i];
        }
        previousSmoothed = smoothed;
    }
    
    // Apply SMA
    magnitudeHistory.push_back(smoothed);
    if (magnitudeHistory.size() > SMA_WINDOW_SIZE) {
        magnitudeHistory.pop_front();
    }
    
    if (magnitudeHistory.size() > 1) {
        std::vector<float> smaResult(smoothed.size(), 0.0f);
        for (const auto& magVec : magnitudeHistory) {
            for (size_t i = 0; i < magVec.size(); i++) {
                smaResult[i] += magVec[i];
            }
        }
        for (size_t i = 0; i < smaResult.size(); i++) {
            smaResult[i] /= magnitudeHistory.size();
        }
        smoothed = smaResult;
    }
    
    return smoothed;
}

void MainWindow::updateHistogram(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return;
    }
    
    int bars_to_show = std::min(MAX_BARS, (int)magnitudes.size());
    float max_magnitude = 0.0f;
    
    for (int i = 0; i < bars_to_show; i++) {
        float mag = magnitudes[i];
        barSet->replace(i, mag);
        if (mag > max_magnitude) {
            max_magnitude = mag;
        }
    }
    
    // Stabilize Y-axis with fixed maximum approach
    if (!maxMagnitudeInitialized && max_magnitude > 0) {
        maxMagnitude = max_magnitude * 1.2f; // 20% padding
        maxMagnitudeInitialized = true;
        histogramAxisY->setRange(0, maxMagnitude);
    } else if (max_magnitude > maxMagnitude * 0.8f) {
        // Only update if new peak exceeds 80% of current max (conservative update)
        if (max_magnitude > maxMagnitude) {
            maxMagnitude = max_magnitude * 1.2f;
            histogramAxisY->setRange(0, maxMagnitude);
        }
    }
}

void MainWindow::updateLinePlot(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return;
    }
    
    // Optimize: Only clear and rebuild if size changed, otherwise replace points
    int points_to_show = std::min(MAX_BARS, (int)magnitudes.size());
    
    // Check if we need to clear (size changed)
    if (lineSeries->count() != points_to_show) {
        lineSeries->clear();
        for (int i = 0; i < points_to_show; i++) {
            lineSeries->append(i, magnitudes[i]);
        }
    } else {
        // Replace existing points (faster than clear + append)
        for (int i = 0; i < points_to_show; i++) {
            lineSeries->replace(i, i, magnitudes[i]);
        }
    }
    
    // Use same stabilized max for consistency
    if (maxMagnitudeInitialized) {
        linePlotAxisY->setRange(0, maxMagnitude);
    }
}

void MainWindow::updateRadial(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return;
    }
    
    // Update the custom radial widget with the same smoothed magnitudes
    radialView->updateData(magnitudes);

    // Use the stabilized maxMagnitude for consistent scaling
    if (maxMagnitudeInitialized && maxMagnitude > 0.0f) {
        radialView->setMaxMagnitude(maxMagnitude);
    }
}

void MainWindow::setPlaybackControlsEnabled(bool enabled) {
    playButton->setEnabled(enabled);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    exportButton->setEnabled(enabled); // Enable export when file is loaded
}

void MainWindow::onLowPassSliderChanged(int value) {
    lowPassLabel->setText(QString::number(value) + " Hz");
    if (lowPassCheckbox->isChecked() && audioPlayer && audioPlayer->getSampleRate() > 0) {
        audioPlayer->setLowPassCutoff(value);
    }
}

void MainWindow::onHighPassSliderChanged(int value) {
    highPassLabel->setText(QString::number(value) + " Hz");
    if (highPassCheckbox->isChecked() && audioPlayer && audioPlayer->getSampleRate() > 0) {
        audioPlayer->setHighPassCutoff(value);
    }
}

void MainWindow::onBandStartSliderChanged(int value) {
    bandStartLabel->setText(QString::number(value) + " Hz");
    if (bandStopCheckbox->isChecked() && audioPlayer && audioPlayer->getSampleRate() > 0) {
        int endValue = bandEndSlider->value();
        if (value < endValue) {
            audioPlayer->setBandStop(value, endValue);
        }
    }
}

void MainWindow::onBandEndSliderChanged(int value) {
    bandEndLabel->setText(QString::number(value) + " Hz");
    if (bandStopCheckbox->isChecked() && audioPlayer && audioPlayer->getSampleRate() > 0) {
        int startValue = bandStartSlider->value();
        if (startValue < value) {
            audioPlayer->setBandStop(startValue, value);
        }
    }
}

void MainWindow::onLowPassCheckboxChanged(int state) {
    if (audioPlayer) {
        audioPlayer->enableLowPass(state == Qt::Checked);
        if (state == Qt::Checked && audioPlayer->getSampleRate() > 0) {
            audioPlayer->setLowPassCutoff(lowPassSlider->value());
        }
    }
}

void MainWindow::onHighPassCheckboxChanged(int state) {
    if (audioPlayer) {
        audioPlayer->enableHighPass(state == Qt::Checked);
        if (state == Qt::Checked && audioPlayer->getSampleRate() > 0) {
            audioPlayer->setHighPassCutoff(highPassSlider->value());
        }
    }
}

void MainWindow::onBandStopCheckboxChanged(int state) {
    if (audioPlayer) {
        audioPlayer->enableBandStop(state == Qt::Checked);
        if (state == Qt::Checked && audioPlayer->getSampleRate() > 0) {
            int startValue = bandStartSlider->value();
            int endValue = bandEndSlider->value();
            if (startValue < endValue) {
                audioPlayer->setBandStop(startValue, endValue);
            }
        }
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    QChartView* view = qobject_cast<QChartView*>(obj);
    RadialVisualizationWidget* radialWidget = qobject_cast<RadialVisualizationWidget*>(obj);
    
    // Handle radial widget separately if needed
    if (radialWidget) {
        // Could add click-and-drag support for radial widget here if needed
        return QMainWindow::eventFilter(obj, event);
    }
    
    if (!view) {
        return QMainWindow::eventFilter(obj, event);
    }
    
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleMousePress(mouseEvent, view);
        return true;
    } else if (event->type() == QEvent::MouseMove) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleMouseMove(mouseEvent, view);
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        handleMouseRelease(mouseEvent, view);
        return true;
    }
    
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::handleMousePress(QMouseEvent* event, QWidget* view) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        activeDragView = view;
        dragStartBin = mouseXToFrequencyBin(event->x(), view, FFT_SIZE);
        dragEndBin = dragStartBin;
    }
}

void MainWindow::handleMouseMove(QMouseEvent* event, QWidget* view) {
    if (isDragging && activeDragView == view) {
        dragEndBin = mouseXToFrequencyBin(event->x(), view, FFT_SIZE);
        // Visual feedback could be added here (highlight selected range)
    }
}

void MainWindow::handleMouseRelease(QMouseEvent* event, QWidget* view) {
    if (event->button() == Qt::LeftButton && isDragging && activeDragView == view) {
        dragEndBin = mouseXToFrequencyBin(event->x(), view, FFT_SIZE);
        
        // Apply band-stop filter to selected range
        if (dragStartBin >= 0 && dragEndBin >= 0 && audioPlayer && audioPlayer->getSampleRate() > 0) {
            int startBin = std::min(dragStartBin, dragEndBin);
            int endBin = std::max(dragStartBin, dragEndBin);
            
            // Convert bins to Hz
            float sampleRate = audioPlayer->getSampleRate();
            float startHz = (startBin * sampleRate) / FFT_SIZE;
            float endHz = (endBin * sampleRate) / FFT_SIZE;
            
            // Update sliders and apply filter
            bandStartSlider->setValue((int)startHz);
            bandEndSlider->setValue((int)endHz);
            bandStopCheckbox->setChecked(true);
            audioPlayer->enableBandStop(true);
            audioPlayer->setBandStop(startHz, endHz);
        }
        
        isDragging = false;
        activeDragView = nullptr;
    }
}

int MainWindow::mouseXToFrequencyBin(int x, QWidget* view, int fftSize) {
    QChartView* chartView = qobject_cast<QChartView*>(view);
    if (chartView) {
        QPointF scenePos = chartView->mapToScene(x, 0);
        QPointF chartPos = chartView->chart()->mapFromScene(scenePos);
        
        // Get chart plot area
        QRectF plotArea = chartView->chart()->plotArea();
        
        // Convert x coordinate to frequency bin
        float normalizedX = (chartPos.x() - plotArea.left()) / plotArea.width();
        int bin = (int)(normalizedX * MAX_BARS);
        
        return std::max(0, std::min(bin, MAX_BARS - 1));
    }
    return 0;
}

QString MainWindow::formatTime(size_t samples, unsigned int sampleRate) {
    if (sampleRate == 0) return "00:00";
    
    double seconds = (double)samples / sampleRate;
    int minutes = (int)(seconds / 60);
    int secs = (int)(seconds) % 60;
    
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                           .arg(secs, 2, 10, QChar('0'));
}

void MainWindow::onPositionChanged(size_t position) {
    // This is called from AudioPlayer when position changes
    // Only update slider if user is not scrubbing
    if (!isUserScrubbing && positionSlider) {
        positionSlider->setValue((int)position);
        updatePositionDisplay();
    }
}

void MainWindow::onPositionSliderChanged(int value) {
    // Update label immediately for visual feedback while scrubbing
    if (isUserScrubbing && audioPlayer) {
        unsigned int sampleRate = audioPlayer->getSampleRate();
        if (sampleRate > 0) {
            positionLabel->setText(formatTime((size_t)value, sampleRate));
        }
    }
}

void MainWindow::onPositionSliderReleased() {
    isUserScrubbing = false;
    if (audioPlayer) {
        // Seek to the position indicated by the slider
        int value = positionSlider->value();
        audioPlayer->seekToPosition((size_t)value);
        updatePositionDisplay();
        // Note: User can manually resume playback with the play button if desired
    }
}

void MainWindow::updatePositionDisplay() {
    if (!audioPlayer) return;
    
    size_t currentPos = audioPlayer->getCurrentPosition();
    size_t totalLength = audioPlayer->getTotalLength();
    unsigned int sampleRate = audioPlayer->getSampleRate();
    
    if (sampleRate > 0 && totalLength > 0) {
        // Update current position label
        positionLabel->setText(formatTime(currentPos, sampleRate));
        
        // Update duration label
        durationLabel->setText(formatTime(totalLength, sampleRate));
        
        // Update slider if not being scrubbed
        if (!isUserScrubbing) {
            positionSlider->setValue((int)currentPos);
        }
    } else {
        positionLabel->setText("00:00");
        durationLabel->setText("00:00");
    }
}

void MainWindow::onVolumeSliderChanged(int value) {
    // Update label
    volumeLabel->setText(QString::number(value) + "%");
    
    // Set volume in audio player (convert from 0-100 to 0.0-1.0)
    if (audioPlayer) {
        float volume = value / 100.0f;
        audioPlayer->setVolume(volume);
    }
}

