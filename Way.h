//
// Created by oreste on 28/10/24.
//
#ifndef WAY_H
#define WAY_H

#include <QGraphicsItem>
#include <QVector>
#include <QMap>
#include <QString>
#include <QPen>
#include <QPainter>

class Way : public QGraphicsItem {
public:
    Way(const QString &id, const QVector<QString> &nodeRefs, const QMap<QString, QString> &tags)
            : wayId(id), nodeRefs(nodeRefs), tags(tags) {}

    QRectF boundingRect() const override {
        // Return a bounding rectangle based on the positions of the nodes
        return QRectF(-5, -5, 10, 10); // Placeholder, adjust according to your needs
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        painter->setPen(getColorForWay());
        painter->setBrush(Qt::NoBrush);

        // Draw the way based on its node references
        for (int i = 0; i < nodeRefs.size() - 1; ++i) {
            QPointF start = getNodePosition(nodeRefs[i]);
            QPointF end = getNodePosition(nodeRefs[i + 1]);
            painter->drawLine(start, end);
        }
    }

private:
    QString wayId;
    QVector<QString> nodeRefs;
    QMap<QString, QString> tags;

    QColor getColorForWay() const {
        if (tags.contains("highway")) {
            return Qt::blue; // Roads
        } else if (tags.contains("building")){
            return Qt::gray; // Buildings
        } else if (tags.contains("waterway")||tags.contains("river")) {
            return Qt::cyan; // Rivers
        } else if (tags.contains("forest")) {
            return Qt::green; // Forest
        } else if (tags.contains("park")) {
            return Qt::darkGreen; // Parks
        } else if (tags.contains("landuse")) {
            if (tags["landuse"] == "residential") {
                return Qt::lightGray; // Residential areas
            }
        }
        return Qt::black; // Default color for other types
    }

    QPointF getNodePosition(const QString &nodeId) const {
        // Implement logic to retrieve node position (x, y) from the scene or node list
        // This is a placeholder implementation; replace it with the actual logic
        return QPointF(0, 0); // Replace with actual logic to get node position
    }
};

#endif // WAY_H
