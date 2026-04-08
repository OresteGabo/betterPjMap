//
// Created by oreste on 29/10/24.
//

#include "CustomGraphicsView.h"
#include "ConfigManager.h"
#include "CustomScene.h"
#include <QPoint>

CustomGraphicsView::CustomGraphicsView(QGraphicsScene* scene, QWidget *parent)
        : QGraphicsView(parent), isDragging(false) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);

    ConfigManager configManager("config.json");
    auto height = configManager.getMainWindowSize().height();
    auto width = configManager.getMainWindowSize().width();
    setFixedHeight(height - 75);
    setFixedWidth(width - 300);

    setScene(scene);
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
}

void CustomGraphicsView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void CustomGraphicsView::wheelEvent(QWheelEvent *event) {
    QPointF mousePos = event->position();
    QPointF mouseScenePos = mapToScene(mousePos.toPoint());
    double scaleFactor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;
    scale(scaleFactor, scaleFactor);
    QPointF newMouseScenePos = mapToScene(mousePos.toPoint());
    QPointF offset = newMouseScenePos - mouseScenePos;
    translate(offset.x(), offset.y());
    event->accept();
}

void CustomGraphicsView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isDragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void CustomGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging) {
        // Pan the view
        QPointF delta = event->pos() - lastMousePos;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        lastMousePos = event->pos();
    }
    QGraphicsView::mouseMoveEvent(event);
}

void CustomGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = false;
        unsetCursor();
    }
    QGraphicsView::mouseReleaseEvent(event);
}
