//
// Created by oreste on 01/01/25.

#include <QtMath>
//

#include "Maille.h"
Maille::Maille(const QPointF &center, double size,bool isCarInside, QGraphicsItem *parent)
        : QGraphicsPolygonItem(parent), isVisible(true) ,isCarInside(isCarInside) {
    // Create the hexagonal shape
    for (int i = 0; i < 6; ++i) {
        double angle = 2 * M_PI / 6 * i;
        hexagon << QPointF(center.x() + size * qCos(angle), center.y() + size * qSin(angle));
    }

    setPolygon(hexagon);

    // Set initial brush and pen
    originalBrush = QBrush(QColor(128, 128, 128, 100)); // Light gray fill with some transparency
    originalPen = QPen(Qt::black, 0.5);                 // Thin black border

    setBrush(originalBrush);
    setPen(originalPen);
}

void Maille::toggleVisibility() {
    if (isVisible) {
        setBrush(Qt::NoBrush); // Remove the background color
        setPen(Qt::NoPen);     // Remove the border
        qDebug() << "Hexagon at" << polygon().boundingRect().center() << "hidden.";
    } else {
        setBrush(originalBrush); // Restore the background color
        setPen(originalPen);     // Restore the border
        qDebug() << "Hexagon at" << polygon().boundingRect().center() << "visible.";
    }
    isVisible = !isVisible; // Toggle the visibility state
}
bool Maille::isPointInside(const QPointF& point) const {
    return hexagon.containsPoint(point, Qt::OddEvenFill);
}

void Maille::setIsCarInside(bool i) {
    isCarInside=i;
}

bool Maille::isCarInside1() const {
    return isCarInside;
}

const QColor &Maille::getColor() const {
    return color;
}

const QPen &Maille::getOriginalPen() const {
    return originalPen;
}

const QBrush &Maille::getOriginalBrush() const {
    return originalBrush;
}
