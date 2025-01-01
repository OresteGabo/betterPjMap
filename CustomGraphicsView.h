//
// Created by oreste on 29/10/24.
//

#ifndef CUSTOMGRAPHICSVIEW_H
#define CUSTOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPointF>
#include <QPoint>
#include <QScrollBar>
#include <QGraphicsScene>

class CustomGraphicsView : public QGraphicsView {
Q_OBJECT

public:
    CustomGraphicsView(QGraphicsScene* scene,QWidget *parent = nullptr);
    //void addIcon(const QString& imagePath, const QPointF& position);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event)override;

private:
    //QPointF lastMousePos;
    bool isDragging;

    double scaleFactor =1.0 ;  // Scale factor for zooming
    QPoint lastMousePos;       // For tracking the last mouse position
    QPoint offset;
};

#endif // CUSTOMGRAPHICSVIEW_H

