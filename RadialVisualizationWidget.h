#ifndef RADIALVISUALIZATIONWIDGET_H
#define RADIALVISUALIZATIONWIDGET_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include <cmath>

class RadialVisualizationWidget : public QWidget {
    Q_OBJECT

public:
    explicit RadialVisualizationWidget(QWidget* parent = nullptr);
    
    void updateData(const std::vector<float>& magnitudes);
    void setMaxMagnitude(float maxMag);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<float> magnitudes;
    float maxMagnitude;
    static constexpr float PI = 3.14159265358979323846f;
    
    void drawPolarPlot(QPainter& painter);
    QPointF polarToCartesian(float angle, float radius, const QPointF& center, float maxRadius);
};

#endif // RADIALVISUALIZATIONWIDGET_H

