#include "RadialVisualizationWidget.h"
#include <QPaintEvent>
#include <algorithm>

RadialVisualizationWidget::RadialVisualizationWidget(QWidget* parent)
    : QWidget(parent), maxMagnitude(1000.0f) {
    setMinimumSize(400, 400);
    setBackgroundRole(QPalette::Base);
}

void RadialVisualizationWidget::updateData(const std::vector<float>& magnitudes) {
    this->magnitudes = magnitudes;
    update(); // Trigger repaint
}

void RadialVisualizationWidget::setMaxMagnitude(float maxMag) {
    maxMagnitude = maxMag;
    update();
}

void RadialVisualizationWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawPolarPlot(painter);
}

void RadialVisualizationWidget::drawPolarPlot(QPainter& painter) {
    QRectF rect = this->rect();
    QPointF center(rect.width() / 2.0, rect.height() / 2.0);
    float maxRadius = std::min(rect.width(), rect.height()) / 2.0f - 20.0f; // Leave some margin
    
    // Draw circular grid
    painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
    for (int i = 1; i <= 4; i++) {
        float radius = (maxRadius * i) / 4.0f;
        painter.drawEllipse(center, radius, radius);
    }
    
    // Draw radial lines (every 30 degrees)
    painter.setPen(QPen(Qt::lightGray, 1));
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30.0f) * PI / 180.0f;
        QPointF end = polarToCartesian(angle, maxRadius, center, maxRadius);
        painter.drawLine(center, end);
    }
    
    // Draw frequency data
    if (magnitudes.empty()) {
        return;
    }
    
    int points_to_show = std::min(64, (int)magnitudes.size());
    
    // Draw filled polygon for the spectrum
    QPolygonF polygon;
    for (int i = 0; i < points_to_show; i++) {
        float angle = (360.0f * i) / points_to_show;
        float angleRad = angle * PI / 180.0f;
        
        // Normalize magnitude to 0-1 range based on maxMagnitude
        float normalizedMag = magnitudes[i] / std::max(maxMagnitude, 1.0f);
        normalizedMag = std::min(normalizedMag, 1.0f); // Clamp to 1.0
        
        float radius = normalizedMag * maxRadius;
        QPointF point = polarToCartesian(angleRad, radius, center, maxRadius);
        polygon.append(point);
    }
    
    // Close the polygon
    if (!polygon.isEmpty()) {
        polygon.append(polygon.first());
    }
    
    // Draw filled area
    QColor fillColor(100, 150, 255, 150); // Semi-transparent blue
    painter.setBrush(QBrush(fillColor));
    painter.setPen(QPen(Qt::blue, 2));
    painter.drawPolygon(polygon);
    
    // Draw outline
    painter.setPen(QPen(Qt::darkBlue, 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(polygon);
    
    // Draw center point
    painter.setBrush(QBrush(Qt::black));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, 3, 3);
}

QPointF RadialVisualizationWidget::polarToCartesian(float angle, float radius, const QPointF& center, float maxRadius) {
    // Convert polar coordinates (angle in radians, radius) to Cartesian
    // Note: In Qt, Y increases downward, so we need to adjust
    float x = center.x() + radius * std::cos(angle - PI / 2.0f); // Rotate -90 degrees to start at top
    float y = center.y() + radius * std::sin(angle - PI / 2.0f);
    return QPointF(x, y);
}

