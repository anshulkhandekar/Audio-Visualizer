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

    // Map a larger subset of magnitudes evenly around the full circle
    int points_to_show = std::min(256, (int)magnitudes.size());
    if (points_to_show <= 0) {
        return;
    }

    // Compute a local max for this frame so the ring \"breathes\" with the music
    float localMax = 0.0f;
    for (int i = 0; i < points_to_show; ++i) {
        localMax = std::max(localMax, magnitudes[i]);
    }
    if (localMax <= 0.0f) {
        return;
    }

    // Small base radius so quiet frames are still visible, but variation stands out
    float baseRadius = maxRadius * 0.05f;
    float dynamicRadius = maxRadius - baseRadius;

    // Draw individual radial spikes
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(Qt::darkBlue, 2));

    for (int i = 0; i < points_to_show; i++) {
        float angleRad = 2.0f * PI * static_cast<float>(i) / static_cast<float>(points_to_show);

        // Normalize magnitude to 0-1 range based on this frame's local max
        float normalizedMag = magnitudes[i] / localMax;
        normalizedMag = std::max(0.0f, std::min(normalizedMag, 1.0f)); // Clamp to [0, 1]

        float radius = baseRadius + normalizedMag * dynamicRadius;

        QPointF endPoint = polarToCartesian(angleRad, radius, center, maxRadius);
        painter.drawLine(center, endPoint);
    }
    
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

