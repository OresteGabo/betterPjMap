#ifndef CUSTOMSCENE_H
#define CUSTOMSCENE_H
#include "Maille.h"
#include <QGraphicsScene>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QDebug>
#include <QPolygonF>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsItem>
#include <QVector>
#include <QTimer>
#include <QJsonObject>
#include <proj.h>
#include "Car.h"

class CustomScene : public QGraphicsScene {
Q_OBJECT



public:
    CustomScene(int width, int height, QObject *parent = nullptr);
    static QPointF latLonToXY(double lat, double lon);
    void updateCarPositions(qreal elapsedTime);
    void startSimulation(); // Start simulation of car movements
    bool isHexGridVisible() const;
    void toggleMailles();
    void updatePolygonColorsBasedOnCarPositions();
    void initializeMailles();
    void updateHexagonCoverage(const QVector<QPointF>& vehiclePositions);
    static QPointF latLonToLambert93(double lat, double lon);
    static QPointF lambert93ToLatLon(double x, double y);
    void drawLambert93Grid(double spacing);

    void updateHexagonsWithCars(const QVector<Car*>& cars);
signals:
    void debugMessage(const QString &message); // Signal to send debug messages

private:
    static QJsonObject loadJsonFile(const QString &configFileName="config.json");
    void loadNodesFromDatabase();
    void loadWaysFromDatabase();
    void loadSpecificWays(const QString &type, const QColor &color);
    void moveCar(QGraphicsEllipseItem *car, const QString &currentNodeId, const QString &nextNodeId, double speed);
    QString decideNextNode(const QString &currentNodeId);
    //QString decideNextNode(const QString &currentNodeId, const QString &previousNodeId);

    QVector<QGraphicsEllipseItem*> carItems; // Stores car graphics items
    QVector<QString> getNeighbors(const QString &nodeId); // Get neighboring nodes
    double getLat(const QString &nodeId); // Get latitude of a node
    double getLon(const QString &nodeId); // Get longitude of a node

    QMap<QString, QVector<QString>> adjacencyList; // Adjacency list for nodes
    QMap<QString, QPointF> nodeMap; // Map for node positions (lat, lon to QPointF)

    void drawHexGrid();
    QGraphicsItemGroup* hexGridGroup; // Group to manage the hexagons
    bool isGridVisible;

    void clearHexGrid();
    QVector<Maille*> mailles;
    QVector<QGraphicsPolygonItem *> polygons; // Store the polygon items

};

#endif // CUSTOMSCENE_H
