//
// Created by oreste on 29/10/24.
//

#include "CustomGraphicsView.h"
#include "ConfigManager.h"
#include "CustomScene.h"
#include <QPoint>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGraphicsItem>
#include <QGraphicsProxyWidget>
#include <QToolTip>

CustomGraphicsView::CustomGraphicsView(QGraphicsScene* scene,QWidget *parent)
        : QGraphicsView(parent), isDragging(false) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);



    ConfigManager configManager("config.json");
    auto height=configManager.getMainWindowSize().height();
    auto width=configManager.getMainWindowSize().width();
    setFixedHeight(height-75);
    setFixedWidth(width-300);

    setScene(scene);
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio); // Fit scene into view


}
void CustomGraphicsView::showEvent(QShowEvent *event) {
    QGraphicsView::showEvent(event);
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
void CustomGraphicsView::wheelEvent(QWheelEvent *event) {
    // Get the current mouse position in scene coordinates
    QPointF mouseScenePos = mapToScene(event->pos());

    // Determine zoom factor
    double scaleFactor = (event->angleDelta().y() > 0) ? 1.1 : 0.9;

    // Apply the zoom
    scale(scaleFactor, scaleFactor);

    // Calculate new mouse position in scene coordinates after scaling
    QPointF newMouseScenePos = mapToScene(event->pos());

    // Compute the difference (offset) between the old and new positions
    QPointF offset = newMouseScenePos - mouseScenePos;

    // Translate the view to keep the mouse position consistent
    translate(offset.x(), offset.y());

    // Accept the event
    event->accept();
}



void CustomGraphicsView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        isDragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);

    //for lambert93
    QPointF scenePoint = mapToScene(event->pos());
    QPointF lambert93Coords = CustomScene::latLonToLambert93(scenePoint.y(), scenePoint.x());
    qDebug() << "Lambert 93 Coordinates:" << lambert93Coords;

    // Optionally show in a tooltip
    QToolTip::showText(event->globalPos(), QString("Lambert 93: (%1, %2)").arg(lambert93Coords.x()).arg(lambert93Coords.y()));
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
