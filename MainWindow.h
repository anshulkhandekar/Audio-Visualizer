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
#include <vector>
#include "AudioPlayer.h"

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
    void onFFTDataReady(const std::vector<float>& magnitudes);
    void onPlaybackFinished();

private:
    void setupUI();
    void updateChart(const std::vector<float>& magnitudes);
    void setPlaybackControlsEnabled(bool enabled);
    
    // UI Components
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;
    QHBoxLayout* buttonLayout;
    QPushButton* loadButton;
    QPushButton* playButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QLabel* statusLabel;
    
    // Chart components
    QChartView* chartView;
    QChart* chart;
    QBarSeries* barSeries;
    QBarSet* barSet;
    QBarCategoryAxis* axisX;
    QValueAxis* axisY;
    
    // Audio player
    AudioPlayer* audioPlayer;
    
    // Chart data
    static constexpr int MAX_BARS = 64; // Display first 64 bars for better performance
};

#endif // MAINWINDOW_H

