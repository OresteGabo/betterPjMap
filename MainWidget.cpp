#include "MainWidget.h"
//#include "Hexagon.h"
#include <QLabel>
#include <fstream>
#include <QIntValidator>
#include <QRandomGenerator>
MainWidget::MainWidget(QWidget *parent) : QWidget(parent) {
    QJsonObject jsonObj = ConfigManager::loadJsonFile();
    QJsonObject screenObj = jsonObj.value("MainWindow").toObject();

    int width = screenObj.value("width").toInt();
    int height = screenObj.value("height").toInt();

    // Initialize the scene and view
    scene = new CustomScene(width, height);
    graphicsView = new CustomGraphicsView(scene, this);

    // Initialize the debug text area
    debugTextArea = new QTextEdit();
    debugTextArea->setReadOnly(true);

    // Create buttons with icons
    restart = new QPushButton();
    restart->setIcon(QIcon("icons/restart.png"));
    restart->setIconSize(QSize(32, 32));
    restart->setFixedSize(50, 50);

    afficherMailles = new QPushButton();
    afficherMailles->setIcon(QIcon("icons/hexagons.png"));
    afficherMailles->setIconSize(QSize(32, 32));
    afficherMailles->setFixedSize(50, 50);

    // Initialize the slider
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 10);

    // Label to display slider value
    auto speedLabel = new QLabel(QString::number(slider->value()) + " X + rapide");
    speedLabel->setAlignment(Qt::AlignCenter);

    // Update the label when the slider value changes
    connect(slider, &QSlider::valueChanged, [speedLabel](int value) {
        speedLabel->setText(QString::number(value) + "X + rapide");
    });

    // Layout for label and slider
    auto sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(speedLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(slider);

    // Connect button signals
    connect(restart, &QPushButton::clicked, this, &MainWidget::restartClicked);
    connect(afficherMailles, &QPushButton::clicked, this, &MainWidget::toggleMailles);

    connect(slider, &QSlider::valueChanged, this, &MainWidget::sliderValueChanged);

    // Top layout with buttons and slider
    auto topLayout = new QHBoxLayout;
    topLayout->addWidget(restart);
    topLayout->addWidget(afficherMailles);
    topLayout->addLayout(sliderLayout);

    // Initialize additional buttons
    runButton = new QPushButton("Start", this);
    displayInfo = new QPushButton("Display Info", this);
    clearButton = new QPushButton("Clear");
    addCarsButton = new QPushButton("Add Cars");

    // Connect additional button signals
    connect(runButton, &QPushButton::clicked, this, &MainWidget::onRunButtonClicked);
    connect(displayInfo, &QPushButton::clicked, this, &MainWidget::onDisplayInfo);
    connect(clearButton, &QPushButton::clicked, this, &MainWidget::clearDebugText);
    connect(addCarsButton, &QPushButton::clicked, this, &MainWidget::onAddCars);

    // Cars input layout
    carsCount = new QLineEdit();
    carsCount->setPlaceholderText("Number of Cars");
    carsCount->setValidator(new QIntValidator(1, 1000, this));

    auto addCarLayout = new QHBoxLayout;
    addCarLayout->addWidget(carsCount);
    addCarLayout->addWidget(addCarsButton);

    // Debug area layout
    auto debugLayout = new QVBoxLayout();
    debugLayout->addLayout(topLayout, 0);
    debugLayout->addWidget(debugTextArea, 1);
    debugLayout->addWidget(runButton, 0);

    auto infoLayout = new QHBoxLayout;
    infoLayout->addWidget(clearButton);
    infoLayout->addWidget(displayInfo);
    debugLayout->addLayout(infoLayout, 0);
    debugLayout->addLayout(addCarLayout, 0);

    // Main layout
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(graphicsView, 3);
    mainLayout->addLayout(debugLayout, 1);

    // Animation timer
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWidget::updateAnimation);

    // Set the final layout
    setLayout(mainLayout);
    setFixedSize(width - 50, height - 100);

    adjacencyList=DatabaseManager::buildNodesAdjacencyList();
    initializeNodeMap();

}

void MainWidget::restartClicked() {
    // Stop animation
    if (animationTimer->isActive()) {
        animationTimer->stop();
    }

    qDebug() << "Restart simulation";

    // Remove all cars from the scene
    for (auto &car : cars) {
        scene->removeItem(car);
        delete car;
    }
    cars.clear();

    // Reset connections
    for (auto line : connectionLines) {
        scene->removeItem(line);
        delete line;
    }
    connectionLines.clear();
    connections.clear();

    // Clear debug area
    debugTextArea->clear();

    // Add new cars with new destinations
    onAddCars();

    // Restart animation
    animationTimer->start(16);
}



void MainWidget::clearDebugText() {
    debugTextArea->clear();
}

void MainWidget::onRunButtonClicked() {
    if (animationTimer->isActive()) {
        animationTimer->stop();
        runButton->setText("Start Simulation");
    } else {
        animationTimer->start(16);
        runButton->setText("Stop Simulation");
    }
}
/*
void MainWidget::onAddCars() {
    int numberOfCars = carsCount->text().toInt();

    for (int i = 0; i < numberOfCars; ++i) {
        while (true) {
            // Randomly select start and end nodes
            QStringList nodes = adjacencyList.keys();
            QString startNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];
            QString endNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];

            if (startNode == endNode) {
                continue; // Avoid assigning the same node as start and end
            }

            // Find a path between the nodes
            QVector<QString> nodePath = DatabaseManager::findPath(startNode, endNode, adjacencyList);

            if (!nodePath.isEmpty()) {
                // Convert node IDs to positions
                QVector<QPointF> path;
                for (const QString &nodeId : nodePath) {
                    path.append(DatabaseManager::getPositionByNodeId(nodeId));
                }

                // Create and assign the car
                QPointF initialPosition = path.first();

                auto car = new Car(QString::number(cars.size() + 1), initialPosition);
                car->setPath(path, nodePath); // Assign the path to the car

                connect(car, &Car::reachedEndOfPath, this, [=](const QString &lastNodeId) {
                    handleCarPathCompletion(car, lastNodeId);
                });

                cars.append(car);
                scene->addItem(car);
                qDebug() << "Car" << car->getId() << "added with path from" << startNode << "to" << endNode;
                break; // Path successfully assigned, exit retry loop
            }

            qDebug() << "No path found between" << startNode << "and" << endNode << ", retrying...";
        }
    }

    onDisplayInfo(); // Update debug text area
    graphicsView->scene()->update(); // Refresh the scene
}
*/


void MainWidget::onAddCars() {
    int numberOfCars = carsCount->text().toInt();

    for (int i = 0; i < numberOfCars; ++i) {
        while (true) {
            QStringList nodes = adjacencyList.keys();
            QString startNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];
            QString endNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];

            if (startNode == endNode) {
                continue; // Avoid same start and end
            }

            QVector<QString> nodePath = DatabaseManager::findPath(startNode, endNode, adjacencyList);

            if (!nodePath.isEmpty()) {
                QVector<QPointF> path;
                for (const QString &nodeId : nodePath) {
                    path.append(DatabaseManager::getPositionByNodeId(nodeId));
                }

                QPointF initialPosition = path.first();
                auto car = new Car(QString::number(cars.size() + 1), initialPosition);
                car->setPath(path, nodePath);

                connect(car, &Car::reachedEndOfPath, this, [=](const QString &lastNodeId) {
                    handleCarPathCompletion(car, lastNodeId);
                });

                cars.append(car);
                scene->addItem(car);

                qDebug() << "Car" << car->getId() << "added with path from" << startNode << "to" << endNode;
                break; // Path successfully assigned
            }

            qDebug() << "No path found between" << startNode << "and" << endNode << ", retrying...";
        }
    }

    onDisplayInfo(); // Update debug text area
    graphicsView->scene()->update(); // Refresh the scene
}





void MainWidget::handleCarPathCompletion(Car *car, const QString &lastNodeId) {
    QString nextWayId = DatabaseManager::findNextWay(lastNodeId);
    if (nextWayId.isEmpty()) {
        qDebug() << "No next way found for car:" << car->getId();
        return;
    }

    QVector<QString> nodes = DatabaseManager::getNodesOfWay(nextWayId);
    QVector<QPointF> newPath;
    for (const auto &nodeId : nodes) {
        newPath.append(DatabaseManager::getPositionByNodeId(nodeId));
    }

    car->setPath(newPath);
}


void MainWidget::onDisplayInfo() {
    debugTextArea->clear();

     for (const auto& car : cars) {
        debugTextArea->append("Car ID: " + car->getId());
        debugTextArea->append("Position: (" + QString::number(car->pos().x()) + ", " + QString::number(car->pos().y()) + ")");
        debugTextArea->append("Speed: " + QString::number(car->getSpeed()));
        debugTextArea->append("Frequency: " + QString::number(car->getFrequency()));
    }

}

void MainWidget::updateAnimation() {
    qreal elapsedTime = animationTimer->interval() / 1000.0; // Time step in seconds
    for (auto &car : cars) {
        car->updatePosition(elapsedTime);
    }
    updateConnections();

    scene->updateHexagonsWithCars(cars);
    scene->update(); // Refresh the scene
}


void MainWidget::sliderValueChanged() {
    qDebug() << "Slider value changed";
}

void MainWidget::initializeNodeMap() {
    QSqlQuery query;

    // Get all nodes belonging to drivable ways
    query.prepare(
            "SELECT DISTINCT n.id, n.lat, n.lon "
            "FROM nodes n "
            "JOIN ways_nodes wn ON n.id = wn.node_id "
            "JOIN tags t ON wn.way_id = t.element_id "
            "WHERE t.tag_key = 'highway'");

    if (!query.exec()) {
        qDebug() << "Failed to query nodes of drivable ways:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        QString nodeId = query.value(0).toString(); // Node ID
        double lat = query.value(1).toDouble();    // Latitude
        double lon = query.value(2).toDouble();    // Longitude

        // Convert lat/lon to scene coordinates
        QPointF position = CustomScene::latLonToXY(lat, lon);

        // Store in nodeMap
        nodeMap[nodeId] = position;
    }

    qDebug() << "Initialized nodeMap with" << nodeMap.size() << "nodes from drivable ways.";
}


void MainWidget::toggleMailles() {
    // Simply delegate the toggle request to the scene
    scene->toggleMailles();
}
void MainWidget::updateConnections() {
    connections.clear(); // Reset connections

    for (int i = 0; i < cars.size(); ++i) {
        for (int j = i + 1; j < cars.size(); ++j) {
            if (cars[i]->isWithinFrequencyRange(cars[j])) {
                connections.append(qMakePair(cars[i], cars[j]));
            }
        }
    }

    // Update the visual representation
    drawConnections();
}

void MainWidget::drawConnections() {
    debugTextArea->clear();
    onDisplayConnections();
    // Clear existing connection visuals
    for (QGraphicsLineItem *line : connectionLines) {
        scene->removeItem(line);
        delete line;
    }
    connectionLines.clear();

    // Draw lines for each connection
    for (const auto &connection : connections) {
        Car *car1 = connection.first;
        Car *car2 = connection.second;

        auto *line = new QGraphicsLineItem(QLineF(car1->pos(), car2->pos()));
        line->setPen(QPen(Qt::blue, 2)); // Customize the line color and width
        scene->addItem(line);
        connectionLines.append(line);
    }
}
void MainWidget::onDisplayConnections() {
    debugTextArea->clear();
    for (const auto &connection : connections) {
        Car *car1 = connection.first;
        Car *car2 = connection.second;
        debugTextArea->append("Car " + car1->getId() + " is connected to Car " + car2->getId());
    }
}