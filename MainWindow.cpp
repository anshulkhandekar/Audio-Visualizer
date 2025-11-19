#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileInfo>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), audioPlayer(nullptr) {
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
    
    // Chart setup
    chart = new QChart();
    chart->setTitle("Frequency Spectrum");
    chart->setAnimationOptions(QChart::NoAnimation); // Disable animation for real-time updates
    
    barSeries = new QBarSeries();
    barSet = new QBarSet("Magnitude");
    
    // Initialize with zero values
    QStringList categories;
    for (int i = 0; i < MAX_BARS; i++) {
        *barSet << 0.0;
        categories << QString::number(i);
    }
    
    barSeries->append(barSet);
    chart->addSeries(barSeries);
    
    // X-axis (frequency bins)
    axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("Frequency Bin");
    chart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);
    
    // Y-axis (magnitude)
    axisY = new QValueAxis();
    axisY->setTitleText("Magnitude");
    axisY->setRange(0, 1000); // Initial range, will adjust dynamically
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisY);
    
    // Chart view
    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(400);
    
    mainLayout->addWidget(chartView);
    
    // Window settings
    setWindowTitle("Audio Visualizer");
    resize(800, 600);
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
    
    // Reset chart
    for (int i = 0; i < MAX_BARS; i++) {
        barSet->replace(i, 0.0);
    }
}

void MainWindow::onFFTDataReady(const std::vector<float>& magnitudes) {
    updateChart(magnitudes);
}

void MainWindow::onPlaybackFinished() {
    statusLabel->setText("Playback finished");
    playButton->setEnabled(true);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
    
    // Reset chart
    for (int i = 0; i < MAX_BARS; i++) {
        barSet->replace(i, 0.0);
    }
}

void MainWindow::updateChart(const std::vector<float>& magnitudes) {
    if (magnitudes.empty()) {
        return;
    }
    
    // Update bars (show first MAX_BARS bins)
    int bars_to_show = std::min(MAX_BARS, (int)magnitudes.size());
    float max_magnitude = 0.0f;
    
    for (int i = 0; i < bars_to_show; i++) {
        float mag = magnitudes[i];
        barSet->replace(i, mag);
        if (mag > max_magnitude) {
            max_magnitude = mag;
        }
    }
    
    // Dynamically adjust Y-axis range (with some padding)
    if (max_magnitude > 0) {
        float y_max = max_magnitude * 1.2f; // 20% padding
        axisY->setRange(0, y_max);
    }
}

void MainWindow::setPlaybackControlsEnabled(bool enabled) {
    playButton->setEnabled(enabled);
    pauseButton->setEnabled(false);
    stopButton->setEnabled(false);
}

