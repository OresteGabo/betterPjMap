#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QFileDialog>
#include <QRandomGenerator>
#include <QTimer>
#include <QSlider>
#include <QPlainTextEdit>
#include "CustomGraphicsView.h"
#include "Car.h"

#include "CustomGraphicsView.h"
#include "ConfigManager.h"
#include <QMessageBox>
#include <QGuiApplication>
#include <QLineEdit>
#include "CustomScene.h"
#include "Node.h"
#include "DatabaseManager.h"
#include <QGraphicsLineItem>

class MainWidget : public QWidget {
Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    void updateAnimation();
    void onDisplayInfo();

signals:


private slots:
    void clearDebugText();



public slots:
    void onRunButtonClicked();
    void onAddCars();
    void restartClicked();
    void sliderValueChanged();


private:
    QTextEdit *debugTextArea;
    CustomGraphicsView *graphicsView;
    QPushButton *clearButton;
    QPushButton *restart,*afficherMailles;
    QSlider * slider;
    CustomScene *scene;



    QLineEdit* carsCount;
    QPushButton* addCarsButton;
    QVector<Car*> cars;
    QPushButton *displayInfo;
    QTimer* animationTimer;
    QPushButton* runButton;
    void drawConnections();
    void updateConnections();

private:
    QVector<QGraphicsPolygonItem*> hexagonItems; // Store references to hexagon items
    bool hexagonsVisible = false;               // Track visibility state
    QMap<QString, QVector<QString>> adjacencyList; // Adjacency list for nodes
    QMap<QString, QPointF> nodeMap;

    void initializeNodeMap();

    void toggleHexGrid();

    void handleCarPathCompletion(Car *car, const QString &lastNodeId);
    QList<QPair<Car*, Car*>> connections;
    QList<QGraphicsLineItem*> connectionLines;

    void onDisplayConnections();
};

#endif // MAINWIDGET_H