#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QSlider>
#include "CustomGraphicsView.h"
#include "Car.h"
#include "ConfigManager.h"
#include <QLineEdit>
#include "CustomScene.h"
#include "DatabaseManager.h"
#include <QGraphicsLineItem>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QElapsedTimer>

struct CarSpawnPlan {
    QString id;
    QPointF initialPosition;
    QVector<QPointF> path;
    QVector<QString> nodePath;
};

class MainWidget : public QWidget {
Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    void updateAnimation();
    void onDisplayInfo();
    double calculateWavelength(double frequency);
    double calculateReceivedPower(double transmittedPower, double antennaGainTx, double antennaGainRx, double wavelength, double distance);
    void calculateReceivedPower();

signals:
    void carAdded(Car *car);

private slots:
    void clearDebugText();
    void toggleMailles();


public slots:
    void onRunButtonClicked();
    void onAddCars();
    void restartClicked();
    void sliderValueChanged(int v);




private:
    QTextEdit *debugTextArea;
    CustomGraphicsView *graphicsView;
    QPushButton *clearButton;
    QPushButton *restartButton;
    QPushButton *toggleGridButton;
    QSlider *speedSlider;
    CustomScene *scene;
    QLineEdit* carCountInput;
    QPushButton* addCarsButton;
    QVector<Car*> cars;
    QFutureWatcher<QVector<CarSpawnPlan>>* addCarsWatcher;
    QPushButton *displayInfo;
    QTimer* animationTimer;
    QPushButton* runButton;
    void drawConnections();
    void updateConnections();

private:
    QMap<QString, QVector<QString>> adjacencyList;
    QMap<QString, QPointF> nodeMap;

    void initializeNodeMap();
    void handleCarPathCompletion(Car *car, const QString &lastNodeId);
    QList<QPair<Car*, Car*>> connections;
    QList<QGraphicsLineItem*> connectionLines;

    void onDisplayConnections();
    QElapsedTimer simulationTimer;
    void applyCarSpawnPlans(const QVector<CarSpawnPlan> &plans);
    void startNextCarBatch();
    void updateAddCarsButtonLabel() const;
    int pendingCarsToAdd = 0;
    static constexpr int kCarBatchSize = 10;

};

#endif // MAINWIDGET_H
