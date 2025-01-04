#include "Car.h"
#include <cmath>
#include <QBrush>
#include <QPen>
#include <QDebug>
Car::Car(const QString &id, const QPointF &initialPosition, double speed,double frequency,int puissance,QGraphicsItem *parent)
        : QGraphicsEllipseItem(-1, -1, 2, 2, parent), carId(id), speed(speed), frequency(frequency) , puissance(puissance){
    setBrush(Qt::red);
    setPen(Qt::NoPen);
    setPos(initialPosition);
    updateRadius();
    color = QColor::fromRgb(
            QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256),
            puissance
    );

}

double Car::getSpeed() const {
    return speed;
}

double Car::getFrequency() const {
    return frequency;
}

QString Car::getId() const {
    return carId;
}

QPointF Car::getCurrentPosition() const {
    return pos();
}

void Car::setCarImage(const QPixmap &image) {
    carImage = image;
}

void Car::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Use the bounding rectangle's width as the radius
    int baseRadius = 5; // Minimum radius
    double scalingFactor = 0.5; // Scale frequency to radius
    int radius = baseRadius + static_cast<int>(frequency * scalingFactor);

    // Draw the car's shape
    painter->setBrush(color);
    //painter->setBrush(Qt::Dense7Pattern);
    painter->drawEllipse(QPointF(0, 0), radius, radius);

    // Optional: Draw a small fixed inner circle for better visualization
    painter->drawEllipse(QPointF(0, 0), baseRadius, baseRadius);

    // Draw the car's image
    if (!carImage.isNull()) {
        painter->drawPixmap(-carImage.width() / 2, -carImage.height() / 2, carImage);
    }
}


void Car::setPath(const QVector<QPointF> &newPath, const QVector<QString> &newNodeIds) {
    path = newPath;
    nodePath = newNodeIds;
    currentPathIndex = 0;
}

void Car::updatePosition(qreal elapsedTime) {
    if (currentPathIndex >= path.size()) {
        return; // No more nodes to move to
    }

    QPointF currentPosition = pos();
    QPointF targetPosition = path[currentPathIndex];

    // Calculate movement
    QLineF line(currentPosition, targetPosition);
    qreal distance = speed * elapsedTime;

    if (distance >= line.length()) {
        setPos(targetPosition);
        currentPathIndex++;

        if (currentPathIndex >= path.size()) {
            emit reachedEndOfPath(nodePath.last()); // Emit the last node ID
        }
    } else {
        // Move toward the target node
        qreal dx = distance * (line.dx() / line.length());
        qreal dy = distance * (line.dy() / line.length());
        setPos(currentPosition.x() + dx, currentPosition.y() + dy);
    }
}

void Car::setPath(const QVector<QPointF> &newPath) {
    path = newPath;
    currentPathIndex = 0;
}

void Car::updateRadius() {
    int baseRadius = 5; // Minimum radius
    double scalingFactor = 0.01; // Scale frequency to radius
    int radius = baseRadius + static_cast<int>(frequency * scalingFactor);
    setRect(-radius, -radius, radius * 2, radius * 2);
}

bool Car::isWithinFrequencyRange(const Car *otherCar) const {
    QPointF thisPosition = pos();
    QPointF otherPosition = otherCar->pos();

    double distance = QLineF(thisPosition, otherPosition).length();
    double radius1 = 5 + frequency * 0.5; // Radius of this car
    double radius2 = 5 + otherCar->getFrequency() * 0.5; // Radius of the other car
    double maxRadius = std::max(radius1, radius2);

    return distance <= maxRadius;
}

void Car::setSpeed(double d) {
    speed=d;
}
