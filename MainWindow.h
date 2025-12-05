#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QLabel>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QTabWidget>
#include <QSlider>
#include <QCheckBox>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QTimer>
#include <QMutex>
#include <vector>
#include <deque>
#include "AudioPlayer.h"
#include "FFTAnalyzer.h"
#include "RadialVisualizationWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadFileClicked();
    void onPlayClicked();
    void onPauseClicked();
    void onStopClicked();
    void onExportClicked();
    void onFFTDataReady(const std::vector<float>& magnitudes); // Stores FFT data (called from audio thread)
    void onFFTUpdateTimer(); // Updates charts at fixed rate (called from main thread)
    void onPlaybackFinished();
    void onPositionChanged(size_t position);
    void onPositionSliderChanged(int value);
    void onPositionSliderReleased();
    void updatePositionDisplay();
    void onVolumeSliderChanged(int value);
    QString formatTime(size_t samples, unsigned int sampleRate);

private:
    void setupUI();
    void updateChart(const std::vector<float>& magnitudes);
    void setPlaybackControlsEnabled(bool enabled);
    std::vector<float> smoothMagnitudes(const std::vector<float>& magnitudes);
    
    // Visualization update methods
    void updateHistogram(const std::vector<float>& magnitudes);
    void updateLinePlot(const std::vector<float>& magnitudes);
    void updateRadial(const std::vector<float>& magnitudes);
    
    // Filter control slots
    void onLowPassSliderChanged(int value);
    void onHighPassSliderChanged(int value);
    void onBandStartSliderChanged(int value);
    void onBandEndSliderChanged(int value);
    void onLowPassCheckboxChanged(int state);
    void onHighPassCheckboxChanged(int state);
    void onBandStopCheckboxChanged(int state);
    
    // Mouse event handlers for click-and-drag
    bool eventFilter(QObject* obj, QEvent* event) override;
    void handleMousePress(QMouseEvent* event, QWidget* view);
    void handleMouseMove(QMouseEvent* event, QWidget* view);
    void handleMouseRelease(QMouseEvent* event, QWidget* view);
    int mouseXToFrequencyBin(int x, QWidget* view, int fftSize);
    
    // UI Components
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    QHBoxLayout* buttonLayout;
    QPushButton* loadButton;
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QPushButton* exportButton;
    QLabel* statusLabel;
    
    // Position scrubber
    QSlider* positionSlider;
    QLabel* positionLabel;
    QLabel* durationLabel;
    QTimer* positionUpdateTimer;
    bool isUserScrubbing;
    
    // Volume control
    QSlider* volumeSlider;
    QLabel* volumeLabel;
    
    // Tab widget for visualizations
    QTabWidget* tabWidget;
    
    // Histogram components
    QChartView* histogramView;
    QChart* histogramChart;
    QBarSeries* barSeries;
    QBarSet* barSet;
    QBarCategoryAxis* histogramAxisX;
    QValueAxis* histogramAxisY;
    
    // Line Plot components
    QChartView* linePlotView;
    QChart* linePlotChart;
    QLineSeries* lineSeries;
    QValueAxis* linePlotAxisX;
    QValueAxis* linePlotAxisY;
    
    // Radial visualization components
    RadialVisualizationWidget* radialView;
    
    // Filter controls
    QGroupBox* filterGroup;
    QSlider* lowPassSlider;
    QSlider* highPassSlider;
    QSlider* bandStartSlider;
    QSlider* bandEndSlider;
    QCheckBox* lowPassCheckbox;
    QCheckBox* highPassCheckbox;
    QCheckBox* bandStopCheckbox;
    QLabel* lowPassLabel;
    QLabel* highPassLabel;
    QLabel* bandStartLabel;
    QLabel* bandEndLabel;
    
    // Audio player
    AudioPlayer* audioPlayer;
    
    // Smoothing and stabilization
    std::deque<std::vector<float>> magnitudeHistory; // For SMA
    std::vector<float> previousSmoothed; // For EMA
    float maxMagnitude; // For y-axis stabilization
    bool maxMagnitudeInitialized;
    static constexpr int SMA_WINDOW_SIZE = 5;
    static constexpr float EMA_ALPHA = 0.3f;
    
    // FFT update throttling to prevent signal queue buildup
    std::vector<float> latestFFTData; // Latest FFT data from audio thread
    QMutex fftDataMutex; // Protect latestFFTData from concurrent access
    QTimer* fftUpdateTimer; // Timer to update charts at fixed rate (30 FPS)
    
    // Click-and-drag state
    bool isDragging;
    int dragStartBin;
    int dragEndBin;
    QWidget* activeDragView;
    
    // Chart data
    static constexpr int MAX_BARS = 64; // Display first 64 bars for better performance
    // FFT_SIZE is defined in FFTAnalyzer.h as a macro
};

#endif // MAINWINDOW_H

