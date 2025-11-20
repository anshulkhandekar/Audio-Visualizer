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
      isDragging(false), dragStartBin(-1), dragEndBin(-1), activeDragView(nullptr) {
    setupUI();
    
    // Create audio player
    audioPlayer = new AudioPlayer(this);
    
    // Connect signals
    connect(audioPlayer, &AudioPlayer::fftDataReady, this, &MainWindow::onFFTDataReady);
    connect(audioPlayer, &AudioPlayer::playbackFinished, this, &MainWindow::onPlaybackFinished);
    
    // Connect buttons
    connect(loadButton, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    connect(pauseButton, &QPushButton::clicked, this, &MainWindow::onPauseClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    
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
    
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(playButton);
    buttonLayout->addWidget(pauseButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    // Filter controls
    filterGroup = new QGroupBox("Frequency Filters", this);
    QGridLayout* filterLayout = new QGridLayout(filterGroup);
    
    // Low-pass filter
    lowPassCheckbox = new QCheckBox("Enable", this);
    lowPassSlider = new QSlider(Qt::Horizontal, this);
    lowPassSlider->setRange(0, 1000);
    lowPassSlider->setValue(0);
    lowPassLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("Low-Pass (cut above):", this), 0, 0);
    filterLayout->addWidget(lowPassCheckbox, 0, 1);
    filterLayout->addWidget(lowPassSlider, 0, 2);
    filterLayout->addWidget(lowPassLabel, 0, 3);
    
    // High-pass filter
    highPassCheckbox = new QCheckBox("Enable", this);
    highPassSlider = new QSlider(Qt::Horizontal, this);
    highPassSlider->setRange(0, 1000);
    highPassSlider->setValue(0);
    highPassLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("High-Pass (cut below):", this), 1, 0);
    filterLayout->addWidget(highPassCheckbox, 1, 1);
    filterLayout->addWidget(highPassSlider, 1, 2);
    filterLayout->addWidget(highPassLabel, 1, 3);
    
    // Band-stop filter
    bandStopCheckbox = new QCheckBox("Enable", this);
    bandStartSlider = new QSlider(Qt::Horizontal, this);
    bandStartSlider->setRange(0, 1000);
    bandStartSlider->setValue(0);
    bandStartLabel = new QLabel("0 Hz", this);
    bandEndSlider = new QSlider(Qt::Horizontal, this);
    bandEndSlider->setRange(0, 1000);
    bandEndSlider->setValue(0);
    bandEndLabel = new QLabel("0 Hz", this);
    filterLayout->addWidget(new QLabel("Band-Stop (cut range):", this), 2, 0);
    filterLayout->addWidget(bandStopCheckbox, 2, 1);
    filterLayout->addWidget(new QLabel("Start:", this), 2, 2);
    filterLayout->addWidget(bandStartSlider, 2, 3);
    filterLayout->addWidget(bandStartLabel, 2, 4);
    filterLayout->addWidget(new QLabel("End:", this), 2, 5);
    filterLayout->addWidget(bandEndSlider, 2, 6);
    filterLayout->addWidget(bandEndLabel, 2, 7);
    
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
    } else {
        QMessageBox::critical(this, "Error", "Failed to load audio file: " + filename);
        statusLabel->setText("Failed to load file");
        setPlaybackControlsEnabled(false);
    }
}

void MainWindow::onPlayClicked() {
    if (audioPlayer->startPlayback()) {
        statusLabel->setText("Playing...");
        playButton->setEnabled(false);
        pauseButton->setEnabled(true);
        stopButton->setEnabled(true);
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

void MainWindow::onFFTDataReady(const std::vector<float>& magnitudes) {
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
    
    // Update all visualizations
    updateHistogram(magnitudes);
    updateLinePlot(magnitudes);
    updateRadial(magnitudes);
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
    
    lineSeries->clear();
    int points_to_show = std::min(MAX_BARS, (int)magnitudes.size());
    
    for (int i = 0; i < points_to_show; i++) {
        lineSeries->append(i, magnitudes[i]);
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
    
    // Update the custom radial widget
    radialView->updateData(magnitudes);
    
    // Update max magnitude for scaling
    if (maxMagnitudeInitialized) {
        radialView->setMaxMagnitude(maxMagnitude);
    }
}

void MainWindow::setPlaybackControlsEnabled(bool enabled) {
    playButton->setEnabled(enabled);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
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

