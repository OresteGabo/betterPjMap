//
// Created by oreste on 01/01/25.
//

#ifndef GRAPHICSINQT_MAILLE_H
#define GRAPHICSINQT_MAILLE_H


//#include <QPolygonF>
#include <QPainter>
#include <QGraphicsPolygonItem>
#include <cmath>
#include <QDebug>
#include "Car.h"
class Maille : public QGraphicsPolygonItem {
public:
    explicit Maille(const QPointF &center, double size,bool isCarInside=false, QGraphicsItem *parent = nullptr)
    ;

    void setTransparency(int alphaLevel) { alpha = alphaLevel; }

    void toggleVisibility() ;

    bool isCarInside(const Car &point) const;
    bool getIsVisible()const{return isVisible;}
    void setIsCarInside(bool i);
private:
    QColor color;
public:
    void setColor(const QColor &color);

private:
    int alpha;
    bool isVisible;
    QBrush originalBrush;
    QPen originalPen;
    bool d_isCarInside;
public:
    bool isCarInside() const;

    const QColor &getColor() const;

    const QPen &getOriginalPen() const;

    const QBrush &getOriginalBrush() const;

private:
    QPolygonF hexagon;


};



#endif //GRAPHICSINQT_MAILLE_H
