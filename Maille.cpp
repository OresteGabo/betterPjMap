//
// Created by oreste on 01/01/25.

#include <QtMath>
//

#include "Maille.h"
Maille::Maille(const QPointF &center, double size,bool d_isCarInside, QGraphicsItem *parent)
        : QGraphicsPolygonItem(parent), isVisible(true) ,d_isCarInside(d_isCarInside) ,color(QColor(0, 0, 128, 100)){
    // Create the hexagonal shape
    for (int i = 0; i < 6; ++i) {
        double angle = 2 * M_PI / 6 * i;
        hexagon << QPointF(center.x() + size * qCos(angle), center.y() + size * qSin(angle));
    }

    setPolygon(hexagon);

    // Set initial brush and pen
    //originalBrush = QBrush(QColor(128, 128, 128, 100)); // Light gray fill with some transparency
    //originalBrush=QBrush(color);
    originalPen = QPen(Qt::black, 0.5);                 // Thin black border
    transmittedPower = QRandomGenerator::global()->bounded(0, 15); // Example: Random between 0W and 15.0W
    antennaGain = QRandomGenerator::global()->bounded(1, 15);

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
bool Maille::isCarInside(const Car& car) const {
    return hexagon.containsPoint(car.pos(), Qt::OddEvenFill);
}

void Maille::setIsCarInside(bool i) {
    d_isCarInside=i;
}

bool Maille::isCarInside() const {
    return d_isCarInside;
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

void Maille::setColor(const QColor &color) {
    Maille::color = color;
    originalBrush=QBrush(color);
}
double Maille::getAntennaGain() const {
    return antennaGain;
}

void Maille::setAntennaGain(double gain) {
    antennaGain = gain;
    qDebug() << "Antenna gain set to:" << gain << "for hexagon at" << polygon().boundingRect().center();
}

double Maille::getTransmittedPower() const {
    return transmittedPower;
}

void Maille::setTransmittedPower(double power) {
    transmittedPower = power;
    qDebug() << "Transmitted power set to:" << power << "for hexagon at" << polygon().boundingRect().center();
}

