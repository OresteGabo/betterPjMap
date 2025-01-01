#ifndef CAR_H
#define CAR_H

#include <QString>
#include <QGraphicsEllipseItem>
#include <QVector>
#include <QPointF>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
class Car :public QObject, public QGraphicsEllipseItem {
    Q_OBJECT
public:
    Car(const QString &id, const QPointF &initialPosition,
        double speed=QRandomGenerator::global()->bounded(30, 80.0),
        double frequency=QRandomGenerator::global()->bounded(10, 100),
        int puissance=QRandomGenerator::global()->bounded(0, 255),
        QGraphicsItem *parent = nullptr);

    void updatePosition(qreal elapsedTime, const QVector<Car*> &cars);


    double getSpeed() const;
    double getFrequency() const;
    QString getId() const;
    QPointF getCurrentPosition() const;
    void updatePosition(qreal elapsedTime);
    void setPath(const QVector<QPointF> &path);

    void setCarImage(const QPixmap &image); // Set the car's image
    void setPath(const QVector<QPointF> &newPath, const QVector<QString> &newNodeIds);
    bool isWithinFrequencyRange(const Car *otherCar) const;
signals:
    void carClicked(); // Signal emitted when the car is clicked

    void reachedEndOfPath(const QString &lastNodeId);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        emit carClicked(); // Emit the signal with the car ID
        QGraphicsEllipseItem::mousePressEvent(event); // Call the base class implementation
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QString carId;
    double speed;
    QVector<QPointF> path; // Path as positions
    QVector<QString> nodePath; // Path as node IDs
    double frequency; // Frequency for communication or updates
    int currentPathIndex = 0; // Index of the next target node
    QVector<QString> nodeIds;

    QPixmap carImage; // Car image
    QColor color;
    int puissance;

    void generatePath(QString qString, QString qString1);

    void updateRadius();
};

#endif // CAR_H
