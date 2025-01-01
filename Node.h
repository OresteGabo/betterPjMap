//
// Created by oreste on 28/10/24.
//

#ifndef NODE_H
#define NODE_H

#include <QGraphicsItem>
#include <QPainter>
#include <QString>
#include<QDebug>
#include "ConfigManager.h"
#include "CustomScene.h"
#include <QRandomGenerator>

class Node {
public:
    QString id;
    double lat;
    double lon;
    QPointF position;
    double distanceTo(const Node *other) const;
    Node(const QString &id, double lat, double lon, const QPointF &pos)
            : id(id), lat(lat), lon(lon), position(pos) {
        qDebug() << "Node constructor called with parameters - Id:" << id
                 << ", Latitude:" << lat
                 << ", Longitude:" << lon
                 << ", Position:" << pos;
        QRandomGenerator rd= QRandomGenerator();
        rd.bounded(1);
    }


    [[nodiscard]] QPointF toPoint()const{
        return CustomScene::latLonToXY(lat,lon);
    }
};

#endif // NODE_H

