#include "MainWidget.h"

#include <QApplication>
#include <QIntValidator>
#include <QLabel>
#include <QRandomGenerator>

MainWidget::MainWidget(QWidget *parent) : QWidget(parent) {
    const QJsonObject jsonObj = ConfigManager::loadJsonFile();
    const QJsonObject screenObj = jsonObj.value("MainWindow").toObject();

    setWindowTitle("Reseau V2V");
    const int width = screenObj.value("width").toInt();
    const int height = screenObj.value("height").toInt();

    scene = new CustomScene(width, height);
    graphicsView = new CustomGraphicsView(scene, this);

    debugTextArea = new QTextEdit();
    debugTextArea->setReadOnly(true);

    restartButton = new QPushButton();
    restartButton->setIcon(QIcon("icons/restart.png"));
    restartButton->setIconSize(QSize(32, 32));
    restartButton->setFixedSize(50, 50);

    toggleGridButton = new QPushButton();
    toggleGridButton->setIcon(QIcon("icons/hexagons.png"));
    toggleGridButton->setIconSize(QSize(32, 32));
    toggleGridButton->setFixedSize(50, 50);

    auto *compassLogo = new QLabel(this);
    compassLogo->setFixedHeight(64);
    compassLogo->setFixedWidth(64);
    compassLogo->setPixmap(QPixmap("images/compass.png"));
    compassLogo->move(1450, 50);
    compassLogo->raise();

    auto *osmLogo = new QLabel(this);
    osmLogo->setFixedHeight(530);
    osmLogo->setFixedWidth(530);
    osmLogo->setPixmap(QPixmap("images/osm.png"));
    osmLogo->move(1450, 650);
    osmLogo->raise();

    speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(1, 5);
    speedSlider->setValue(1);

    auto *speedLabel = new QLabel(QString::number(speedSlider->value()) + "X + rapide");
    speedLabel->setAlignment(Qt::AlignCenter);
    connect(speedSlider, &QSlider::valueChanged, this, [speedLabel](int value) {
        speedLabel->setText(QString::number(value) + "X + rapide");
    });
    connect(speedSlider, &QSlider::valueChanged, this, &MainWidget::sliderValueChanged);

    auto *sliderLayout = new QVBoxLayout;
    sliderLayout->addWidget(speedLabel, 0, Qt::AlignCenter);
    sliderLayout->addWidget(speedSlider);

    connect(restartButton, &QPushButton::clicked, this, &MainWidget::restartClicked);
    connect(toggleGridButton, &QPushButton::clicked, this, &MainWidget::toggleMailles);

    auto *topLayout = new QHBoxLayout;
    topLayout->addWidget(restartButton);
    topLayout->addWidget(toggleGridButton);
    topLayout->addLayout(sliderLayout);

    runButton = new QPushButton("Start", this);
    displayInfo = new QPushButton("Display Info", this);
    clearButton = new QPushButton("Clear");
    addCarsButton = new QPushButton("Add Cars");

    connect(runButton, &QPushButton::clicked, this, &MainWidget::onRunButtonClicked);
    connect(displayInfo, &QPushButton::clicked, this, &MainWidget::onDisplayInfo);
    connect(clearButton, &QPushButton::clicked, this, &MainWidget::clearDebugText);
    connect(addCarsButton, &QPushButton::clicked, this, &MainWidget::onAddCars);

    carCountInput = new QLineEdit();
    carCountInput->setPlaceholderText("Number of Cars");
    carCountInput->setValidator(new QIntValidator(1, 1000, this));

    auto *addCarLayout = new QHBoxLayout;
    addCarLayout->addWidget(carCountInput);
    addCarLayout->addWidget(addCarsButton);

    auto *debugLayout = new QVBoxLayout();
    debugLayout->addLayout(topLayout, 0);
    debugLayout->addWidget(debugTextArea, 1);
    debugLayout->addWidget(runButton, 0);

    auto *infoLayout = new QHBoxLayout;
    infoLayout->addWidget(clearButton);
    infoLayout->addWidget(displayInfo);
    debugLayout->addLayout(infoLayout, 0);
    debugLayout->addLayout(addCarLayout, 0);

    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(graphicsView, 3);
    mainLayout->addLayout(debugLayout, 1);

    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &MainWidget::updateAnimation);

    addCarsWatcher = new QFutureWatcher<QVector<CarSpawnPlan>>(this);
    connect(addCarsWatcher, &QFutureWatcher<QVector<CarSpawnPlan>>::finished, this, [this]() {
        applyCarSpawnPlans(addCarsWatcher->result());
        if (pendingCarsToAdd > 0) {
            startNextCarBatch();
        } else {
            addCarsButton->setEnabled(true);
            carCountInput->setEnabled(true);
            addCarsButton->setText("Add Cars");
        }
    });

    setLayout(mainLayout);
    setFixedSize(width - 50, height - 100);

    adjacencyList = DatabaseManager::buildNodesAdjacencyList();
    initializeNodeMap();
    simulationTimer.start();
    sliderValueChanged(speedSlider->value());
}

void MainWidget::restartClicked() {
    const bool wasRunning = animationTimer->isActive();
    if (wasRunning) {
        animationTimer->stop();
    }

    qInfo() << "Restart simulation";

    for (auto *car : cars) {
        scene->removeItem(car);
        delete car;
    }
    cars.clear();

    for (auto *line : connectionLines) {
        scene->removeItem(line);
        delete line;
    }
    connectionLines.clear();
    connections.clear();
    debugTextArea->clear();

    onAddCars();

    if (wasRunning) {
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

void MainWidget::onAddCars() {
    const int numberOfCars = carCountInput->text().toInt();
    if (numberOfCars <= 0) {
        return;
    }

    pendingCarsToAdd += numberOfCars;
    carCountInput->clear();
    addCarsButton->setEnabled(false);
    carCountInput->setEnabled(false);

    if (!addCarsWatcher->isRunning()) {
        startNextCarBatch();
    } else {
        updateAddCarsButtonLabel();
    }
}

void MainWidget::startNextCarBatch() {
    if (pendingCarsToAdd <= 0 || addCarsWatcher->isRunning()) {
        return;
    }

    const int batchSize = std::min(pendingCarsToAdd, kCarBatchSize);
    pendingCarsToAdd -= batchSize;
    updateAddCarsButtonLabel();

    const auto adjacencyCopy = adjacencyList;
    const auto nodeMapCopy = nodeMap;
    const int firstCarIndex = cars.size();

    addCarsWatcher->setFuture(QtConcurrent::run([adjacencyCopy, nodeMapCopy, batchSize, firstCarIndex]() {
        QVector<CarSpawnPlan> plans;
        plans.reserve(batchSize);
        const QStringList nodes = adjacencyCopy.keys();

        if (nodes.size() < 2) {
            return plans;
        }

        constexpr int maxAttemptsPerCar = 100;
        for (int i = 0; i < batchSize; ++i) {
            for (int attempt = 0; attempt < maxAttemptsPerCar; ++attempt) {
                const QString startNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];
                const QString endNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];

                if (startNode == endNode) {
                    continue;
                }

                const QVector<QString> nodePath = DatabaseManager::findPath(startNode, endNode, adjacencyCopy);
                if (nodePath.isEmpty()) {
                    continue;
                }

                QVector<QPointF> path;
                path.reserve(nodePath.size());
                bool missingCoords = false;

                for (const QString &nodeId : nodePath) {
                    if (!nodeMapCopy.contains(nodeId)) {
                        missingCoords = true;
                        break;
                    }
                    path.append(nodeMapCopy[nodeId]);
                }

                if (missingCoords || path.isEmpty()) {
                    continue;
                }

                CarSpawnPlan plan;
                plan.id = QString::number(firstCarIndex + plans.size() + 1);
                plan.initialPosition = path.first();
                plan.path = path;
                plan.nodePath = nodePath;
                plans.append(plan);
                break;
            }
        }

        return plans;
    }));
}

void MainWidget::applyCarSpawnPlans(const QVector<CarSpawnPlan> &plans) {
    for (const auto &plan : plans) {
        auto *car = new Car(plan.id, plan.initialPosition);
        car->setPos(plan.initialPosition);
        car->setPath(plan.path, plan.nodePath);

        connect(car, &Car::reachedEndOfPath, this, [this, car](const QString &lastNodeId) {
            handleCarPathCompletion(car, lastNodeId);
        });

        cars.append(car);
        scene->addItem(car);
    }

    onDisplayInfo();
    scene->update();
}

void MainWidget::updateAddCarsButtonLabel() const {
    if (pendingCarsToAdd > 0) {
        addCarsButton->setText(QString("Adding... (%1 left)").arg(pendingCarsToAdd));
    } else {
        addCarsButton->setText("Adding...");
    }
}

void MainWidget::handleCarPathCompletion(Car *car, const QString &lastNodeId) {
    const QString nextWayId = DatabaseManager::findNextWay(lastNodeId);
    if (nextWayId.isEmpty()) {
        qDebug() << "No next way found for car:" << car->getId();
        return;
    }

    const QVector<QString> nodes = DatabaseManager::getNodesOfWay(nextWayId);
    QVector<QPointF> newPath;
    newPath.reserve(nodes.size());
    for (const auto &nodeId : nodes) {
        newPath.append(DatabaseManager::getPositionByNodeId(nodeId));
    }

    car->setPath(newPath);
}

void MainWidget::onDisplayInfo() {
    debugTextArea->clear();

    for (const auto &car : cars) {
        debugTextArea->append("Car ID: " + car->getId());
        debugTextArea->append("Position: (" + QString::number(car->pos().x()) + ", " + QString::number(car->pos().y()) + ")");
        debugTextArea->append("Speed: " + QString::number(car->getSpeed()));
        debugTextArea->append("Frequency: " + QString::number(car->getFrequency()));
        debugTextArea->append("Puissance: " + QString::number(car->getPuissance()));
        debugTextArea->append("\n");
    }
}

void MainWidget::updateAnimation() {
    const qreal elapsedTime = simulationTimer.restart() / 1000.0;
    for (auto *car : cars) {
        car->updatePosition(elapsedTime);
    }
    updateConnections();

    scene->updateHexagonsWithCars(cars);
    scene->update();
}

void MainWidget::sliderValueChanged(int value) {
    int interval = 1000 / value;
    for (auto *car : cars) {
        car->setSpeed(car->getSpeed() * value);
    }
    if (animationTimer->isActive()) {
        animationTimer->setInterval(interval);
    }
}

void MainWidget::initializeNodeMap() {
    QSqlQuery query;
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
        const QString nodeId = query.value(0).toString();
        const double lat = query.value(1).toDouble();
        const double lon = query.value(2).toDouble();
        nodeMap[nodeId] = CustomScene::latLonToXY(lat, lon);
    }
}

void MainWidget::toggleMailles() {
    scene->toggleMailles();
}

void MainWidget::updateConnections() {
    connections.clear();

    for (int i = 0; i < cars.size(); ++i) {
        for (int j = i + 1; j < cars.size(); ++j) {
            Car *car1 = cars[i];
            Car *car2 = cars[j];
            if (car1->isWithinFrequencyRange(car2)) {
                connections.append(qMakePair(car1, car2));
            }
        }
    }

    drawConnections();
}

double MainWidget::calculateWavelength(double frequency) {
    constexpr double speedOfLight = 3e8;
    return speedOfLight / frequency;
}

double MainWidget::calculateReceivedPower(double transmittedPower, double antennaGainTx, double antennaGainRx, double wavelength, double distance) {
    constexpr double pi = 3.141592653589793;
    const double numerator = transmittedPower * antennaGainTx * antennaGainRx * std::pow(wavelength, 2);
    const double denominator = std::pow(4 * pi * distance, 2);
    return numerator / denominator;
}

void MainWidget::calculateReceivedPower() {
    for (const auto &car : cars) {
        for (const auto &maille : scene->getMailles()) {
            const double distance = QLineF(car->pos(), maille->polygon().boundingRect().center()).length();
            if (distance > 0) {
                const double wavelength = 3e8 / car->getFrequency();
                const double receivedPower = (maille->getTransmittedPower() * maille->getAntennaGain() * car->getAntennaGain() *
                                              std::pow(wavelength, 2)) /
                                             (std::pow(4 * M_PI * distance, 2));
                Q_UNUSED(receivedPower);
            }
        }
    }
}

void MainWidget::drawConnections() {
    debugTextArea->clear();
    onDisplayConnections();

    for (QGraphicsLineItem *line : connectionLines) {
        scene->removeItem(line);
        delete line;
    }
    connectionLines.clear();

    for (const auto &connection : connections) {
        Car *car1 = connection.first;
        Car *car2 = connection.second;

        auto *line = new QGraphicsLineItem(QLineF(car1->pos(), car2->pos()));
        line->setPen(QPen(Qt::blue, 2));
        scene->addItem(line);
        connectionLines.append(line);
    }
}

void MainWidget::onDisplayConnections() {
    debugTextArea->clear();
    for (const auto &connection : connections) {
        Car *car1 = connection.first;
        Car *car2 = connection.second;

        const double distance = QLineF(car1->pos(), car2->pos()).length();
        const double wavelength1 = calculateWavelength(car1->getFrequency());
        const double wavelength2 = calculateWavelength(car2->getFrequency());

        const double transmittedPower1 = car1->getTransmittedPower();
        const double receivedPower1 = calculateReceivedPower(
                transmittedPower1,
                car1->getAntennaGain(),
                car2->getAntennaGain(),
                wavelength1,
                distance
        );

        const double transmittedPower2 = car2->getTransmittedPower();
        const double receivedPower2 = calculateReceivedPower(
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

        debugTextArea->append("<hr>");
        debugTextArea->append(connectionInfo);
        debugTextArea->append(transmittedPowerInfo1);
        debugTextArea->append(receivedPowerInfo1);
        debugTextArea->append(transmittedPowerInfo2);
        debugTextArea->append(receivedPowerInfo2);
        debugTextArea->append(car1Details);
        debugTextArea->append(car2Details);
    }
}
