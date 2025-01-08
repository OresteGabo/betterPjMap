#include "MainWidget.h"
#include <QLabel>
#include <fstream>
#include <QIntValidator>
#include <QRandomGenerator>
#include <QApplication>
MainWidget::MainWidget(QWidget *parent) : QWidget(parent) {
    QJsonObject jsonObj = ConfigManager::loadJsonFile();
    QJsonObject screenObj = jsonObj.value("MainWindow").toObject();
    setWindowTitle("Reseau V2V");
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

    // OpenStreetMap Logo
    auto compassLogo = new QLabel(this);
    compassLogo->setFixedHeight(64);
    compassLogo->setFixedWidth(64);
    QPixmap compassPixmap("images/compass.png"); // Replace with the actual path to your logo file
    //compassLogo->setPixmap(compassPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // Scale image
    compassLogo->setPixmap(compassPixmap);

    compassLogo->move(1450,50);
    compassLogo->raise();

    // OpenStreetMap Logo
    auto osmLogo = new QLabel(this);
    osmLogo->setFixedHeight(530);
    osmLogo->setFixedWidth(530);
    QPixmap logoPixmap("images/osm.png"); // Replace with the actual path to your logo file
    osmLogo->setPixmap(logoPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)); // Scale image
    osmLogo->setPixmap(logoPixmap);

    osmLogo->move(1450,650);
    osmLogo->raise();


    // Initialize the slider
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(1, 5);
    slider->setValue(1);

    // Label to display slider value
    auto speedLabel = new QLabel(QString::number(slider->value()) + "X + rapide");
    speedLabel->setAlignment(Qt::AlignCenter);

    // Update the label when the slider value changes
    connect(slider, &QSlider::valueChanged, [speedLabel](int value) {
        speedLabel->setText(QString::number(value) + "X + rapide");
    });
    // Connect slider to animation speed
    connect(slider, &QSlider::valueChanged, this, &MainWidget::sliderValueChanged);

    // Set the default timer interval based on the slider's default value
    //

    // Layout for label and slider
    auto sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(speedLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(slider);

    // Connect button signals
    connect(restart, &QPushButton::clicked, this, &MainWidget::restartClicked);
    connect(afficherMailles, &QPushButton::clicked, this, &MainWidget::toggleMailles);

    //connect(slider, &QSlider::valueChanged, this, &MainWidget::sliderValueChanged);


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
    simulationTimer.start();
    sliderValueChanged(slider->value());

}

void MainWidget::restartClicked() {
    bool animRunning=animationTimer->isActive();
    // Stop animation
    if (animationTimer->isActive()) {
        animationTimer->stop();
    }

    qInfo() << "Restart simulation";

    int totalCars= cars.size();

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

    if(animRunning) {
        // Restart animation
        animationTimer->start(16);
    }
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


/**
 * While loop inside the function ensure that as long as the path is not found,
 * the code is re-executed again, so other startNode and endnode are picked and recalculate the path again
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

            if (!nodePath.isEmpty() || nodePath.size()>400) {
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

                // Update the scene immediately after adding the car
                scene->update();          // Redraw the scene
                QApplication::processEvents();
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
         debugTextArea->append("Puissance: " + QString::number(car->getPuissance()));
         debugTextArea->append("\n");
    }

}

void MainWidget::updateAnimation() {

    qreal elapsedTime = simulationTimer.restart() / 1000.0; // Time step in seconds
    for (auto &car : cars) {
        car->updatePosition(elapsedTime);
    }
    updateConnections();

    scene->updateHexagonsWithCars(cars);
    scene->update(); // Refresh the scene
}


void MainWidget::sliderValueChanged(int value) {
    int interval = 1000 / value; // Example: higher value -> faster speed (smaller interval)
    for (auto &car : cars) {
        car->setSpeed(car->getSpeed()*value);
    }
    if (animationTimer->isActive()) {
        animationTimer->setInterval(interval); // Adjust the timer's interval dynamically
    }

    qDebug() << "Slider value changed to:" << value << ", Timer interval set to:" << interval;

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
/*
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
*/
void MainWidget::updateConnections() {
    constexpr double connectionThreshold = 1e-10; // Example threshold for received power

    connections.clear();

    for (int i = 0; i < cars.size(); ++i) {
        for (int j = i + 1; j < cars.size(); ++j) {
            Car* car1 = cars[i];
            Car* car2 = cars[j];
            if (car1->isWithinFrequencyRange(car2)) {
                double distance = QLineF(car1->pos(), car2->pos()).length(); // Distance between cars
                double wavelength = calculateWavelength(car1->getFrequency());
                double receivedPower = calculateReceivedPower(
                        car1->getTransmittedPower(),
                        car1->getAntennaGain(),
                        car2->getAntennaGain(),
                        wavelength,
                        distance
                );

                //if (receivedPower >= connectionThreshold) {
                    connections.append(qMakePair(car1, car2));
                //}
            }


        }
    }

    drawConnections();
}

double MainWidget::calculateWavelength(double frequency) {
    constexpr double speedOfLight = 3e8; // Speed of light in m/s
    return speedOfLight / frequency;
}
double MainWidget::calculateReceivedPower(double transmittedPower, double antennaGainTx, double antennaGainRx, double wavelength, double distance) {
    constexpr double pi = 3.141592653589793;
    double numerator = transmittedPower * antennaGainTx * antennaGainRx * std::pow(wavelength, 2);
    double denominator = std::pow(4 * pi * distance, 2);
    return numerator / denominator;
}
void MainWidget::calculateReceivedPower() {
    for (const auto &car : cars) {
        for (const auto &maille : scene->getMailles()) { // Assuming you have a getMailles() method in CustomScene
            double distance = QLineF(car->pos(), maille->polygon().boundingRect().center()).length();
            if (distance > 0) {
                double wavelength = 3e8 / car->getFrequency(); // Speed of light / frequency
                double receivedPower = (maille->getTransmittedPower() * maille->getAntennaGain() * car->getAntennaGain() *
                                        std::pow(wavelength, 2)) /
                                       (std::pow(4 * M_PI * distance, 2));
                qDebug() << "Received power at car" << car->getId() << "from maille at"
                         << maille->polygon().boundingRect().center() << ":" << receivedPower << "W";
            }
        }
    }
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

        // Calculate distance between cars
        double distance = QLineF(car1->pos(), car2->pos()).length();

        // Calculate wavelength for car1 and car2 frequencies
        double wavelength1 = calculateWavelength(car1->getFrequency());
        double wavelength2 = calculateWavelength(car2->getFrequency());

        // Calculate transmitted and received power for both directions
        double transmittedPower1 = car1->getTransmittedPower();
        double receivedPower1 = calculateReceivedPower(
                transmittedPower1,
                car1->getAntennaGain(),
                car2->getAntennaGain(),
                wavelength1,
                distance
        );

        double transmittedPower2 = car2->getTransmittedPower();
        double receivedPower2 = calculateReceivedPower(
                transmittedPower2,
                car2->getAntennaGain(),
                car1->getAntennaGain(),
                wavelength2,
                distance
        );

        QString connectionInfo = QString(
                "<span style='background-color: rgba(30, 30, 30, 0.8); color: white; "
                "padding: 2px 5px; border-radius: 5px;'>"
                "V %1</span> &lt;--&gt; "
                "<span style='background-color: rgba(30, 30, 30, 0.8); color: white; "
                "padding: 2px 5px; border-radius: 5px;'>V %2</span>"
        )
                .arg(car1->getId())
                .arg(car2->getId());

        QString transmittedPowerInfo1 = QString(
                "<span style='background-color: rgba(20, 60, 20, 0.8); color: white; padding: 2px 5px;'>"
                "↑ V%2: Transmise : %1 W</span>"
        )
                .arg(transmittedPower1, 0, 'f', 6)
                .arg(car1->getId());
        QString transmittedPowerInfo2 = QString(
                "<span style='background-color: rgba(20, 60, 20, 0.8); color: white; padding: 2px 5px;'>"
                "↑ V%2: Transmise : %1 W</span>"
        )
                .arg(transmittedPower2, 0, 'f', 6)
                .arg(car2->getId());

        QString receivedPowerInfo1 = QString(
                "<span style='background-color: rgba(60, 20, 20, 0.8); color: white; padding: 2px 5px;'>"
                "↓ Reçue : %1 W</span>"
        )
                .arg(receivedPower1, 0, 'f', 6);
        QString receivedPowerInfo2 = QString(
                "<span style='background-color: rgba(60, 20, 20, 0.8); color: white; padding: 2px 5px;'>"
                "↓ Reçue : %1 W</span>"
        )
                .arg(receivedPower2, 0, 'f', 6);

        QString car1Details = QString("    <b>Voiture %1</b> : Fréquence : %2 Hz, Gain d'antenne : %3")
                .arg(car1->getId())
                .arg(car1->getFrequency(), 0, 'f', 2)
                .arg(car1->getAntennaGain(), 0, 'f', 2);
        QString car2Details = QString("    <b>Voiture %1</b> : Fréquence : %2 Hz, Gain d'antenne : %3")
                .arg(car2->getId())
                .arg(car2->getFrequency(), 0, 'f', 2)
                .arg(car2->getAntennaGain(), 0, 'f', 2);

        debugTextArea->append("<hr>"); // Separator for readability
        debugTextArea->append(connectionInfo);
        debugTextArea->append(transmittedPowerInfo1);
        debugTextArea->append(receivedPowerInfo1);

        debugTextArea->append(transmittedPowerInfo2);
        debugTextArea->append(receivedPowerInfo2);
        debugTextArea->append(car1Details);
        debugTextArea->append(car2Details);

    }
}
