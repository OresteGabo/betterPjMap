
#include "CustomScene.h"
#include "Car.h"
#include <QtConcurrent/QtConcurrent>
#include <QSqlError>
CustomScene::CustomScene(int width, int height, QObject *parent)
        :QGraphicsScene(parent), hexGridGroup(nullptr), isGridVisible(false)  {
    setSceneRect(0, 0, width, height);
    setBackgroundBrush(QBrush(Qt::lightGray));
    loadWaysFromDatabase();
}

void CustomScene::loadNodesFromDatabase() {
    QSqlQuery query;
    query.exec("SELECT id, lat, lon, isImportant FROM nodes");

    while (query.next()) {
        QString nodeId = query.value(0).toString();
        double lat = query.value(1).toDouble();
        double lon = query.value(2).toDouble();
        bool isImportant = query.value(3).toBool();
        if (isImportant) {
            QPointF position = latLonToXY(lat, lon);
            QGraphicsEllipseItem *nodeItem = addEllipse(position.x() - 0.2, position.y() - 0.2, 0.4, 0.4, QPen(Qt::blue),
                                                        QBrush(Qt::blue));
            nodeItem->setData(0, nodeId);
        }
    }
}

void CustomScene::loadWaysFromDatabase() {
    // Define QFutureWatcher objects
    auto highwayWatcher = new QFutureWatcher<void>(this);
    auto waterWatcher = new QFutureWatcher<void>(this);
    auto buildingWatcher = new QFutureWatcher<void>(this);
    auto landuseWatcher = new QFutureWatcher<void>(this);

    auto updateScene = [this]() {
        qDebug() << "Scene updated after a task completed.";
        this->update();
    };

    connect(highwayWatcher, &QFutureWatcher<void>::finished, this, updateScene);
    connect(highwayWatcher, &QFutureWatcher<void>::finished, this, updateScene);
    connect(waterWatcher, &QFutureWatcher<void>::finished, this, updateScene);
    connect(buildingWatcher, &QFutureWatcher<void>::finished, this, updateScene);
    connect(landuseWatcher, &QFutureWatcher<void>::finished, this, updateScene);

    highwayWatcher->setFuture(QtConcurrent::run([this]() {
        loadSpecificWays("highway", QColor(64, 64, 64));
    }));
    waterWatcher->setFuture(QtConcurrent::run([this]() {
        loadSpecificWays("waterway", QColor(0, 150, 255, 120));
    }));

    buildingWatcher->setFuture(QtConcurrent::run([this]() {
        loadSpecificWays("building", QColor(150, 75, 0, 150));
    }));

    landuseWatcher->setFuture(QtConcurrent::run([this]() {
        loadSpecificWays("landuse", QColor(34, 139, 34, 150));
    }));
}
/*
void CustomScene::loadSpecificWays(const QString &type, const QColor &color) {
    QString connectionName = QString("%1_connection").arg(type);
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName); // Unique connection name
    db.setDatabaseName("OSMData");
    db.setHostName("127.0.0.1");
    db.setUserName("oreste");
    db.setPassword("Muhirehonore@1*");

    if (!db.open()) {
        qDebug() << "Database connection failed for" << type << ":" << db.lastError().text();
        return;
    }

    emit debugMessage("Started loading " + type);
    QSqlQuery query(db);
    query.prepare("SELECT id FROM ways WHERE id IN (SELECT element_id FROM tags WHERE tag_key = :type)");
    query.bindValue(":type", type);
    query.exec();

    QVector<QPolygonF> polygons;
    QVector<QPainterPath> paths;

    while (query.next()) {
        QString wayId = query.value(0).toString();
        QVector<QPointF> wayPoints;

        QSqlQuery wayQuery(db);
        wayQuery.prepare("SELECT node_id FROM ways_nodes WHERE way_id = :wayId ORDER BY node_order");
        wayQuery.bindValue(":wayId", wayId);
        wayQuery.exec();

        while (wayQuery.next()) {
            QString nodeId = wayQuery.value(0).toString();

            QSqlQuery nodeQuery(db);
            nodeQuery.prepare("SELECT lat, lon FROM nodes WHERE id = :id");
            nodeQuery.bindValue(":id", nodeId);

            if (!nodeQuery.exec()) {
                qDebug() << "Node query execution failed:" << nodeQuery.lastError().text();
                continue;
            }

            if (nodeQuery.next()) {
                double lat = nodeQuery.value(0).toDouble();
                double lon = nodeQuery.value(1).toDouble();
                QPointF pos = latLonToXY(lat, lon);
                wayPoints.append(pos);
            }
        }

        if (!wayPoints.isEmpty()) {
            if (wayPoints.first() == wayPoints.last()) {
                polygons.append(QPolygonF(wayPoints));
            } else {
                QPainterPath path;
                path.moveTo(wayPoints.first());
                for (const QPointF &point : wayPoints) {
                    path.lineTo(point);
                }
                paths.append(path);
            }
        }
    }

    // Close the database connection
    db.close();
    QSqlDatabase::removeDatabase(connectionName);

    // Update GUI in the main thread
    QMetaObject::invokeMethod(this, [this, type, color, polygons, paths]() {
        QPen pen(color);
        if (type != "highway") {
            pen.setCosmetic(true);
        }

        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);

        for (const auto &polygon : polygons) {
            addPolygon(polygon, pen, QBrush(color));
        }
        for (const auto &path : paths) {
            addPath(path, pen, QBrush(Qt::NoBrush));
        }

        qDebug() << "Finished loading" << type << "ways.";
        this->update();
    }, Qt::QueuedConnection);

    emit debugMessage("Finished loading " + type);
}
*/
void CustomScene::loadSpecificWays(const QString &type, const QColor &color) {
    // Create a unique database connection for this query
    QString connectionName = QString("%1_connection").arg(type);
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
    db.setDatabaseName("OSMData");
    db.setHostName("127.0.0.1");
    db.setUserName("oreste");
    db.setPassword("Muhirehonore@1*");

    if (!db.open()) {
        qDebug() << "Database connection failed for" << type << ":" << db.lastError().text();
        return;
    }

    emit debugMessage("Started loading " + type);

    // Fetch ways of the specified type
    QSqlQuery query(db);
    query.prepare("SELECT id FROM ways WHERE id IN (SELECT element_id FROM tags WHERE tag_key = :type)");
    query.bindValue(":type", type);

    if (!query.exec()) {
        qDebug() << "Query execution failed for ways of type" << type << ":" << query.lastError().text();
        db.close();
        QSqlDatabase::removeDatabase(connectionName);
        return;
    }

    QVector<QPolygonF> polygons; // Store geometry
    QVector<QPainterPath> paths; // Store non-closed paths
    QVector<QGraphicsPolygonItem*> polygonItems; // Store scene items

    while (query.next()) {
        QString wayId = query.value(0).toString();
        QVector<QPointF> wayPoints;

        // Fetch nodes for the current way
        QSqlQuery wayQuery(db);
        wayQuery.prepare("SELECT node_id FROM ways_nodes WHERE way_id = :wayId ORDER BY node_order");
        wayQuery.bindValue(":wayId", wayId);

        if (!wayQuery.exec()) {
            qDebug() << "Way query execution failed for way ID" << wayId << ":" << wayQuery.lastError().text();
            continue;
        }

        while (wayQuery.next()) {
            QString nodeId = wayQuery.value(0).toString();

            // Fetch latitude and longitude of the node
            QSqlQuery nodeQuery(db);
            nodeQuery.prepare("SELECT lat, lon FROM nodes WHERE id = :id");
            nodeQuery.bindValue(":id", nodeId);

            if (nodeQuery.exec() && nodeQuery.next()) {
                double lat = nodeQuery.value(0).toDouble();
                double lon = nodeQuery.value(1).toDouble();
                QPointF pos = latLonToXY(lat, lon);
                wayPoints.append(pos);
            } else {
                qDebug() << "Node query failed for node ID" << nodeId << ":" << nodeQuery.lastError().text();
            }
        }

        // Determine if the way forms a closed polygon or a path
        if (!wayPoints.isEmpty()) {
            if (wayPoints.first() == wayPoints.last()) {
                polygons.append(QPolygonF(wayPoints)); // Closed polygon
            } else {
                QPainterPath path;
                path.moveTo(wayPoints.first());
                for (const QPointF &point : wayPoints) {
                    path.lineTo(point);
                }
                paths.append(path); // Open path
            }
        }
    }

    // Close and remove the database connection
    db.close();
    QSqlDatabase::removeDatabase(connectionName);

    // Update the scene in the main thread
    QMetaObject::invokeMethod(this, [this, type, color, polygons, paths, &polygonItems]() {
        QPen pen(color);
        pen.setJoinStyle(Qt::RoundJoin);
        pen.setCapStyle(Qt::RoundCap);

        for (const auto &polygon : polygons) {
            auto polygonItem = addPolygon(polygon, pen, QBrush(color));
            polygonItems.append(polygonItem); // Keep track of added polygon items
        }
        for (const auto &path : paths) {
            addPath(path, pen, QBrush(Qt::NoBrush));
        }

        qDebug() << "Finished loading" << type << "ways.";
        this->update();
    }, Qt::QueuedConnection);

    emit debugMessage("Finished loading " + type);
}

QPointF CustomScene::latLonToXY(double lat, double lon) {
    QJsonObject jsonObj = loadJsonFile();
    QJsonObject boundObj = jsonObj.value("Bound").toObject();
    QJsonObject screenObj = jsonObj.value("MainWindow").toObject();

    int width = screenObj.value("width").toInt();
    int height = screenObj.value("height").toInt();
    double minLat = boundObj.value("minLat").toDouble();
    double maxLat = boundObj.value("maxLat").toDouble();
    double minLon = boundObj.value("minLon").toDouble();
    double maxLon = boundObj.value("maxLon").toDouble();

    double x = (lon - minLon) / (maxLon - minLon) * width;
    double y = height - (lat - minLat) / (maxLat - minLat) * height;

    return QPointF(x, y);
}

QJsonObject CustomScene::loadJsonFile(const QString &configFileName) {
    QFile file(configFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open JSON file:" << configFileName;
        return QJsonObject();
    }
    QByteArray jsonData = file.readAll();
    QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));
    return jsonDoc.object();
}

void CustomScene::addCar(const QString &carId, const QPointF &position) {
    /*double speed = QRandomGenerator::global()->bounded(30, 101);       // Random speed
    double frequency = QRandomGenerator::global()->bounded(1, 10);    // Random frequency
    auto *car = new Car(carId, position, speed, frequency);
    addItem(car);
    qDebug()<<"Add item called";
    carItems.append(car);*/
}


void CustomScene::updateCarPositions(qreal elapsedTime) {
    for (auto carItem : carItems) {
        QPointF currentPosition = carItem->pos();
        QPointF newPosition = currentPosition + QPointF(1.0 * elapsedTime, 0);
        carItem->setPos(newPosition);
    }
}

void CustomScene::moveCar(QGraphicsEllipseItem *car, const QString &currentNodeId, const QString &nextNodeId, double speed) {
    QPointF currentPos = latLonToXY(getLat(currentNodeId), getLon(currentNodeId));
    QPointF nextPos = latLonToXY(getLat(nextNodeId), getLon(nextNodeId));

    QPointF direction = nextPos - currentPos;
    double length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
    direction /= length;

    QPointF newPosition = car->pos() + direction * speed * 0.1;
    car->setPos(newPosition);

    if ((newPosition - nextPos).manhattanLength() < 1.0) {
        QString newTarget = decideNextNode(nextNodeId);
        moveCar(car, nextNodeId, newTarget, speed);
    }
}

QString CustomScene::decideNextNode(const QString &currentNodeId) {
    auto neighbors = getNeighbors(currentNodeId);

    if (neighbors.size() == 1) {
        return neighbors.first();
    }

    return neighbors[qrand() % neighbors.size()];
}

void CustomScene::startSimulation() {
    auto timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        for (auto car : carItems) {
            moveCar(car, car->data(0).toString(), "nextNodeId", 50.0);
        }
    });
    timer->start(100);
}
QVector<QString> CustomScene::getNeighbors(const QString &nodeId) {
    return adjacencyList[nodeId];
}

double CustomScene::getLat(const QString &nodeId) {
    // Query the database or use a preloaded map
    return nodeMap[nodeId].y();
}

double CustomScene::getLon(const QString &nodeId) {
    return nodeMap[nodeId].x();
}



void CustomScene::toggleHexGrid() {
    if (isGridVisible) {
        clearHexGrid();
    } else {
        drawHexGrid();
    }
    isGridVisible = !isGridVisible;
}

bool CustomScene::isHexGridVisible() const {
    return isGridVisible;
}

void CustomScene::drawHexGrid() {
    if (hexGridGroup) {
        qDebug() << "Hex grid is already drawn.";
        return;
    }

    hexGridGroup = new QGraphicsItemGroup();
    const double hexSize = 35.0; // Size of each hexagon (side length)
    const double xOffset = 1.5 * hexSize;     // Horizontal spacing between centers
    const double yOffset = sqrt(3) * hexSize; // Vertical spacing between centers

    int rows = height() / yOffset + 2; // Adjust rows to ensure complete coverage
    int cols = width() / xOffset + 2;  // Adjust columns to ensure complete coverage

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            double x = col * xOffset;
            double y = row * yOffset + (col % 2) * (yOffset / 2);

            // Create hexagon
            QPolygonF hexagon;
            for (int i = 0; i < 6; ++i) {
                double angle = 2 * M_PI / 6 * i;
                hexagon << QPointF(x + hexSize * qCos(angle), y + hexSize * qSin(angle));
            }

            auto hexItem = new QGraphicsPolygonItem(hexagon);
            hexItem->setPen(QPen(Qt::black, 0.5)); // Thin gray lines for the grid
            hexItem->setBrush(Qt::NoBrush);
            hexGridGroup->addToGroup(hexItem);
        }
    }

    addItem(hexGridGroup);
    qDebug() << "Hex grid drawn.";
}



void CustomScene::clearHexGrid() {
    if (hexGridGroup) {
        removeItem(hexGridGroup);
        delete hexGridGroup;
        hexGridGroup = nullptr;
        qDebug() << "Hex grid cleared.";
    }
}

void CustomScene::updatePolygonColorsBasedOnCarPositions() {
    for (auto polygonItem : polygons) {
        QString carsInside;
        bool carInside = false;

        for (const auto &item : carItems) {
            // Safely cast the item to Car
            Car *car = dynamic_cast<Car *>(item);
            if (car && polygonItem->polygon().containsPoint(car->pos(), Qt::OddEvenFill)) {
                carInside = true;
                carsInside += car->getId() + " "; // Now we can safely access getId()
            }
        }


        // Update the polygon's color
        if (carInside) {
            polygonItem->setBrush(QColor(0, 0, 150, 120)); // Highlight color
            qDebug() << "Polygon contains cars: " << carsInside;
        } else {
            polygonItem->setBrush(Qt::NoBrush); // Reset to transparent
        }
    }
}
