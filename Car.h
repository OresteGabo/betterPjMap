#ifndef CAR_H
#define CAR_H

#include <QString>
#include <QGraphicsEllipseItem>
#include <QVector>
#include <QPointF>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

class Car : public QObject, public QGraphicsEllipseItem {
    Q_OBJECT

public:
    Car(const QString &id, const QPointF &initialPosition,
        double speed=QRandomGenerator::global()->bounded(10, 40),
        double frequency=QRandomGenerator::global()->bounded(10, 100),
        int puissance=QRandomGenerator::global()->bounded(0, 255),
        QGraphicsItem *parent = nullptr);

    double getAntennaGain() const;
    double getTransmittedPower() const;
    double getSpeed() const;
    double getFrequency() const;
    QString getId() const;
    QPointF getCurrentPosition() const;
    void updatePosition(qreal elapsedTime);
    void setPath(const QVector<QPointF> &path);
    void setCarImage(const QPixmap &image);
    void setPath(const QVector<QPointF> &newPath, const QVector<QString> &newNodeIds);
    bool isWithinFrequencyRange(const Car *otherCar) const;
    void setSpeed(double d);

signals:
    void carClicked();
    void reachedEndOfPath(const QString &lastNodeId);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        emit carClicked();
        QGraphicsEllipseItem::mousePressEvent(event);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QString carId;
    double speed;
    QVector<QPointF> path;
    QVector<QString> nodePath;
    double frequency;
    int currentPathIndex = 0;
    QPixmap carImage;
    QColor color;
    int puissance;
    void updateRadius();
    double transmittedPower;
    double antennaGain;

public:
    const QColor &getColor() const;
    int getPuissance() const;
};

#endif // CAR_H
