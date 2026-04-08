#include "CustomScene.h"

#include "Car.h"
#include <QtConcurrent/QtConcurrent>
#include <QSqlError>
#include <cmath>

CustomScene::CustomScene(int width, int height, QObject *parent)
        : QGraphicsScene(parent),isGridVisible(false) {
    setSceneRect(0, 0, width, height);
    //setBackgroundBrush(QBrush(Qt::lightGray));
    setBackgroundBrush(QBrush(QColor(220,220,220)));
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

void CustomScene::loadSpecificWays(const QString &type, const QColor &color) {
    // TODO(11): CustomScene should receive already-loaded geometry from a
    // repository/service instead of opening its own database connections.
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
    // TODO(07): Avoid the N+1 query pattern below by loading all node
    // coordinates for a way, or all ways of a type, with joined SQL.
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
            QMetaObject::invokeMethod(this, [this, type, color, wayPoints]() {
                if (wayPoints.first() == wayPoints.last()) {
                    // Closed way - render as a polygon
                    addPolygon(QPolygonF(wayPoints), QPen(Qt::NoPen), QBrush(color));
                } else {
                    // Open way - render as a polyline
                    QPainterPath path;
                    path.moveTo(wayPoints.first());
                    for (const QPointF &point : wayPoints) {
                        path.lineTo(point);
                    }
                    addPath(path, QPen(color, 1, Qt::SolidLine), QBrush(Qt::NoBrush));
                }
                this->update();
            }, Qt::QueuedConnection);
        }
    }

    db.close();
    QSqlDatabase::removeDatabase(connectionName);

    emit debugMessage("Finished loading " + type);
}

void CustomScene::loadWaysFromDatabase() {
    auto highwayWatcher = new QFutureWatcher<void>(this);
    auto waterWatcher = new QFutureWatcher<void>(this);
    auto buildingWatcher = new QFutureWatcher<void>(this);
    auto landuseWatcher = new QFutureWatcher<void>(this);

    auto cleanupWatcher = [](QFutureWatcher<void>* watcher) {
        watcher->deleteLater();
    };

    connect(highwayWatcher, &QFutureWatcher<void>::finished, this, [this, highwayWatcher, cleanupWatcher]() {
        emit debugMessage("Highways loaded.");
        cleanupWatcher(highwayWatcher);
    });

    connect(waterWatcher, &QFutureWatcher<void>::finished, this, [this, waterWatcher, cleanupWatcher]() {
        emit debugMessage("Waterways loaded.");
        cleanupWatcher(waterWatcher);
    });

    connect(buildingWatcher, &QFutureWatcher<void>::finished, this, [this, buildingWatcher, cleanupWatcher]() {
        emit debugMessage("Buildings loaded.");
        cleanupWatcher(buildingWatcher);
    });

    connect(landuseWatcher, &QFutureWatcher<void>::finished, this, [this, landuseWatcher, cleanupWatcher]() {
        emit debugMessage("Landuse loaded.");
        cleanupWatcher(landuseWatcher);
    });

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

// Initialize the hexagons (mailles)
void CustomScene::initializeMailles() {
    const double hexSize = 35.0; // Hexagon size
    const double xOffset = 1.5 * hexSize; // Horizontal spacing
    const double yOffset = sqrt(3) * hexSize; // Vertical spacing

    int rows = height() / yOffset + 2; // Number of rows
    int cols = width() / xOffset + 2;  // Number of columns

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            double x = col * xOffset;
            double y = row * yOffset + (col % 2) * (yOffset / 2); // Offset for staggered grid

            auto maille = new Maille(QPointF(x, y), hexSize);
            addItem(maille); // Add to scene
            mailles.append(maille); // Store reference in vector
        }
    }

    qDebug() << "Initialized hexagonal grid with" << mailles.size() << "hexagons.";
}

QPointF CustomScene::latLonToXY(double lat, double lon) {
    // TODO(12): Cache bounds and screen dimensions in memory. Reading
    // config.json on every coordinate conversion is expensive.
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

void CustomScene::toggleMailles() {
    if (mailles.isEmpty()) {
        initializeMailles(); // Only initialize if not already done
    }

    // Ensure that the visibility state aligns with `isGridVisible`
    for (auto maille : mailles) {
        if (isGridVisible) {
            maille->setVisible(false); // Hide if currently visible
        } else {
            maille->setVisible(true);  // Show if currently hidden
        }
    }

    isGridVisible = !isGridVisible; // Toggle the visibility state
    update();
}
/*
void CustomScene::updateHexagonsWithCars(const QVector<Car*>& cars) {
    for (auto& maille : mailles) {
        maille->setIsCarInside(false);  // Reset the flag for each hexagon

        for (const auto& car : cars) {
            if(maille->isCarInside(*car)){

                maille->setColor(car->getColor());
                break;
            }else{
                maille->setColor(QColor(0,0,0,0));
                maille->setBrush(maille->getOriginalBrush());
            }
        }

        if(maille->getIsVisible()){
            if (maille->isCarInside()) {
                maille->setBrush(QBrush(maille->getColor()));  // Example: Highlight with red if a car is inside
            } else {
                maille->setBrush(maille->getOriginalBrush());  // Reset to the original brush
            }
        }
    }
}*/
void CustomScene::updateHexagonsWithCars(const QVector<Car *> &cars) {
    // TODO(18): Move hexagon coverage and RF calculations into a
    // simulation/service layer so the scene stays focused on rendering only.
    for (auto& maille : mailles) {
        maille->setIsCarInside(false);  // Reset the flag for each hexagon

        for (const auto& car : cars) {
            if(maille->isCarInside(*car)){

                maille->setColor(car->getColor());
                break;
            }else{
                maille->setColor(QColor(0,0,0,0));
                maille->setBrush(maille->getOriginalBrush());
            }
        }

        if(maille->getIsVisible()){
            if (maille->isCarInside()) {
                maille->setBrush(QBrush(maille->getColor()));  // Example: Highlight with red if a car is inside
            } else {
                maille->setBrush(maille->getOriginalBrush());  // Reset to the original brush
            }
        }
    }

    // Calculate the received power for all cars from mailles
    updateReceivedPower(cars);
}

void CustomScene::updateReceivedPower(const QVector<Car *> &cars) {
    for (const auto &car : cars) {
        for (const auto &maille : mailles) {
            double distance = QLineF(car->pos(), maille->polygon().boundingRect().center()).length();
            if (distance > 0) {
                // Calculate wavelength (Speed of light / Frequency)
                double wavelength = 3e8 / car->getFrequency();

                // Calculate received power using the Friis transmission equation
                double receivedPower = (maille->getTransmittedPower() * maille->getAntennaGain() * car->getAntennaGain() *
                                        std::pow(wavelength, 2)) /
                                       (std::pow(4 * M_PI * distance, 2));

                qDebug() << "Received power at car" << car->getId() << "from maille at"
                         << maille->polygon().boundingRect().center() << ":" << receivedPower << "W";

                // Optional: You could store or process this value further here
            }
        }
    }
}

const QVector<Maille *> &CustomScene::getMailles() const {
    return mailles;
}
