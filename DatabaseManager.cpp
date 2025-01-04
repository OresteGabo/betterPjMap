#include <QQueue>
#include <QRandomGenerator>
#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(
        const QString& dbName,
        const QString& user,
        const QString& password,
        const QString& fileName)
{
    initialiseDatabase(dbName, user, password);
    //parseData(fileName);
    ConfigManager cf = ConfigManager();
    calculateAndSaveBoundsToConfig(cf);
}

bool DatabaseManager::parseNodes(QXmlStreamReader& xml, QSqlQuery& query) {
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == "node") {
                QString id = xml.attributes().value("id").toString();
                double lat = xml.attributes().value("lat").toDouble();
                double lon = xml.attributes().value("lon").toDouble();

                // Insert the node into the nodes table
                query.prepare("INSERT INTO nodes (id, lat, lon) VALUES (:id, :lat, :lon)");
                query.bindValue(":id", id);
                query.bindValue(":lat", lat);
                query.bindValue(":lon", lon);

                if (!query.exec()) {
                    qDebug() << "Failed to insert node:" << query.lastError().text();
                    return false;
                }
            }
        } else if (xml.isEndElement() && xml.name() == "osm") {
            break; // End of parsing when reaching the end of the OSM element
        }
    }
    return true;
}

bool DatabaseManager::parseWays(QXmlStreamReader& xml, QSqlQuery& query) {
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == "way") {
            QString wayId = xml.attributes().value("id").toString();
            QVector<QString> nodeRefs;
            QMap<QString, QString> tags;

            while (!(xml.isEndElement() && xml.name() == "way")) {
                xml.readNext();
                if (xml.isStartElement() && xml.name() == "nd") {
                    nodeRefs.append(xml.attributes().value("ref").toString());
                } else if (xml.isStartElement() && xml.name() == "tag") {
                    tags.insert(xml.attributes().value("k").toString(), xml.attributes().value("v").toString());
                }
            }

            // Insert way
            query.prepare("INSERT INTO ways (id, visible) VALUES (:id, :visible)");
            query.bindValue(":id", wayId);
            query.bindValue(":visible", 0);

            if (!query.exec()) {
                qDebug() << "Failed to insert way:" << query.lastError().text();
                return false;
            }

            // Insert tags
            for (const auto& key : tags.keys()) {
                query.prepare("INSERT INTO tags (element_id, tag_key, value) VALUES (:id, :key, :value)");
                query.bindValue(":id", wayId);
                query.bindValue(":key", key);
                query.bindValue(":value", tags[key]);
                if (!query.exec()) {
                    qDebug() << "Failed to insert tag for way:" << query.lastError().text();
                }
            }

            // Insert way-node relationships
            int order = 0;
            for (const auto& ref : nodeRefs) {
                query.prepare("INSERT INTO ways_nodes (way_id, node_id, node_order) VALUES (:wayId, :nodeId, :order)");
                query.bindValue(":wayId", wayId);
                query.bindValue(":nodeId", ref);
                query.bindValue(":order", order++);
                if (!query.exec()) {
                    qDebug() << "Failed to insert way-node relationship:" << query.lastError().text();
                }
            }
        }
    }
    return true;
}

// Step 2: Parse OSM file data into the database
bool DatabaseManager::parseData(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << fileName;
        return false;
    }
    QXmlStreamReader xml(&file);
    QSqlQuery query;

    // Critical section: Database operations
    truncateAllTables();
    createTables();

    // Parse nodes
    parseNodes(xml, query);

    // Reset the XML reader before parsing ways
    file.seek(0);
    xml.setDevice(&file);

    parseWays(xml,query);


    if (xml.hasError()) {
        qDebug() << "XML Parsing Error:" << xml.errorString();
        return false;
    }

    file.close();
    return true;
}

void DatabaseManager::createTables() {
    QSqlQuery query;
    db.transaction();

    // Query for creating nodes table
    query.exec("CREATE TABLE IF NOT EXISTS nodes ("
               "id VARCHAR(255) PRIMARY KEY,"
               "lat DOUBLE NOT NULL,"
               "lon DOUBLE NOT NULL"
               ") ENGINE=InnoDB;");

    // Query for creating ways table
    query.exec("CREATE TABLE IF NOT EXISTS ways ("
               "id VARCHAR(255) PRIMARY KEY,"
               "visible BOOLEAN"
               ") ENGINE=InnoDB;");

    // Query for creating ways_nodes table
    query.exec("CREATE TABLE IF NOT EXISTS ways_nodes ("
               "way_id VARCHAR(255),"
               "node_id VARCHAR(255),"
               "node_order INT,"
               "PRIMARY KEY (way_id, node_id, node_order),"
               "FOREIGN KEY (way_id) REFERENCES ways(id) ON DELETE CASCADE,"
               "FOREIGN KEY (node_id) REFERENCES nodes(id) ON DELETE CASCADE"
               ") ENGINE=InnoDB;");

    // Query for creating tags table
    query.exec("CREATE TABLE IF NOT EXISTS tags ("
               "element_id VARCHAR(255),"
               "tag_key VARCHAR(255),"
               "value TEXT,"
               "PRIMARY KEY (element_id, tag_key)"
               ") ENGINE=InnoDB;");

    db.commit();
}


QMap<QString, QString> DatabaseManager::getWayTags(const QString &wayId) {
    QMap<QString, QString> tags;
    QSqlQuery query;

    // Query to get all tags for the specified way
    query.prepare("SELECT tag_key, value FROM tags WHERE element_id = :wayId ;");
    query.bindValue(":wayId", wayId);

    if (query.exec()) {
        while (query.next()) {
            auto key = query.value(0).toString();
            auto value = query.value(1).toString();
            tags.insert(key, value);
        }
    } else {
        qDebug() << "Failed to retrieve tags for way" << wayId << ":" << query.lastError();
    }

    return tags;
}

QVector<QString> DatabaseManager::getDrivableWaysIds() {
    QVector<QString> wayIds;

    // Define the query to get drivable ways that also have a name
    QSqlQuery query;
    QString sql = R"(
        SELECT DISTINCT element_id
        FROM tags
        WHERE tag_key = 'highway'
    )";

    // Execute the query
    if (query.exec(sql)) {
        // Fetch the result and store the way IDs
        while (query.next()) {
            QString wayId = query.value(0).toString();
            wayIds.push_back(wayId);
        }
    } else {
        qDebug() << "Query execution failed:" << query.lastError().text();
    }

    return wayIds;
}

QString DatabaseManager::getWayNameById(const QString& wayId) {
    QString wayName;


    // Prepare the SQL query
    QSqlQuery query;
    query.prepare(R"(
        SELECT DISTINCT tags.value
        FROM tags
        JOIN ways ON tags.element_id = ways.id
        WHERE tags.tag_key = 'name'
          AND ways.id = :wayId
    )");

    // Bind the way ID parameter
    query.bindValue(":wayId", wayId);

    // Execute the query and fetch the result
    if (query.exec()) {
        if (query.next()) {
            wayName = query.value(0).toString();
        } else {
            qDebug() << "No name found for way ID:" << wayId;
        }
    } else {
        qDebug() << "Query execution failed:" << query.lastError().text();
    }

    return wayName;
}
QVector<QString> DatabaseManager::getNodesOfWay(const QString& wayId) {
    QVector<QString> nodeIds;

    // SQL query to fetch the node IDs of the given way, ordered by node_order
    QSqlQuery query;
    QString sql = R"(
        SELECT node_id
        FROM ways_nodes
        WHERE way_id = :wayId
        ORDER BY node_order
    )";

    // Prepare and bind the way ID parameter
    query.prepare(sql);
    query.bindValue(":wayId", wayId);

    // Execute the query and fetch the node IDs
    if (query.exec()) {
        while (query.next()) {
            QString nodeId = query.value(0).toString();
            nodeIds.push_back(nodeId);
        }
    } else {
        qDebug() << "Failed to retrieve nodes for way ID" << wayId << ":" << query.lastError().text();
    }

    return nodeIds;
}

AdjacencyList DatabaseManager::buildAdjacencyList() {
    AdjacencyList adjList;

    QString sql="SELECT distinct element_id FROM tags WHERE tag_key='highway'";
    QSqlQuery query;
    query.prepare(sql);
    query.exec();
    QVector<QString> drivableWayIDS;
    if (query.exec()) {
        while (query.next()) {
            QString nodeId = query.value(0).toString();
            drivableWayIDS.push_back(nodeId);
        }
    } else {

    }

    // Step 2: Iterate through each drivable way
    for (const auto& wayId : drivableWayIDS) {
        // Get the list of nodes for this way
        auto roads=getNodesOfWay(wayId);
        if(roads.size()>7){
            adjList[wayId]=roads;
        }
    }
    return adjList;
}

///TODO : from this , i want to be able to generate a random path on the go, while running, so that the user can just run cars, and cars get only nodes, and nodes generate paths while running
AdjacencyList DatabaseManager::buildDrivableAdjacencyList() {
    QMap<QString, QVector<QString>> adjacencyList;

    QSqlQuery query;

    // Step 1: Select all nodes in drivable ways (e.g., tagged as 'highway')
    query.prepare(
            "SELECT wn.node_id, wn.way_id, wn.node_order "
            "FROM ways_nodes wn "
            "JOIN tags t ON wn.way_id = t.element_id "
            "WHERE t.tag_key = 'highway' "
            "ORDER BY wn.way_id, wn.node_order");

    if (!query.exec()) {
        qDebug() << "Failed to query drivable ways and nodes:" << query.lastError().text();
        return adjacencyList;
    }

    // Step 2: Populate adjacency list
    QString lastWayId;
    QString previousNodeId;
    while (query.next()) {
        QString nodeId = query.value(0).toString();
        QString wayId = query.value(1).toString();

        if (lastWayId != wayId) {
            // Reset for a new way
            lastWayId = wayId;
            previousNodeId.clear();
        }

        if (!previousNodeId.isEmpty()) {
            // Add bidirectional edge between the previous and current node
            adjacencyList[previousNodeId].append(nodeId);
            adjacencyList[nodeId].append(previousNodeId);
        }

        // Update the previous node
        previousNodeId = nodeId;
    }

    qDebug() << "Adjacency list created with" << adjacencyList.size() << "nodes.";
    return adjacencyList;
}

QString DatabaseManager::getWayNameForNode(const QString &nodeId) {
    QSqlQuery query;
    query.prepare(R"(
        SELECT t.value AS way_name
        FROM ways_nodes wn
        JOIN tags t ON wn.way_id = t.element_id
        WHERE wn.node_id = :nodeId
          AND t.tag_key = 'name'
          AND (t.value LIKE '%rue%' OR t.value LIKE '%avenue%' OR t.value LIKE '%boulevard%')
        LIMIT 1;
    )");

    query.bindValue(":nodeId", nodeId);

    if (!query.exec()) {
        qDebug() << "Database query failed:" << query.lastError().text();
        return QString();
    }

    if (query.next()) {
        return query.value(0).toString(); // Return the way name
    }

    return QString(); // Return an empty string if no result is found
}

QPointF DatabaseManager::getPositionByNodeId(QString nodeId){
    QSqlQuery sql;
    QString quertString=R"(
        select lat,lon from nodes where id = :nodeId
    )";
    sql.prepare(quertString);
    sql.bindValue(":nodeId", nodeId);
    if(sql.exec()){
        if (sql.next()) {
            double lat = sql.value(0).toDouble();
            double lon = sql.value(1).toDouble();
            return CustomScene::latLonToXY(lat, lon);
        }
    }
    qDebug()<<"The pos of node "+nodeId+" is not found";
    return {5,5};
}

QVector<QString> DatabaseManager::getNodesOfWaysWithName() {
    return QVector<QString>();
}

void DatabaseManager::initialiseDatabase(const QString &dbName, const QString &user, const QString &password) {

    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);
    if (!db.open()) {
        qDebug() << db.lastError().text();
    }
}
QString DatabaseManager::findNextWay(const QString& lastNode) {
    QSqlQuery query;

    // SQL query to find a way where the first node matches the lastNode
    query.prepare(R"(
        SELECT ways_nodes.way_id
        FROM ways_nodes
        WHERE node_id = :lastNode
        AND node_order = 0
    )");
    query.bindValue(":lastNode", lastNode);

    if (query.exec() && query.next()) {
        // Return the first matching way_id
        return query.value(0).toString();
    }

    qDebug() << "No connected way found for last node:" << lastNode;
    return QString(); // Return an empty string if no way is found
}

QString DatabaseManager::getRandomDrivableWay() {
    QSqlQuery query;
    query.prepare(R"(
        SELECT DISTINCT ways.id
        FROM ways
        JOIN tags ON ways.id = tags.element_id
        WHERE tags.tag_key = 'highway'
          AND tags.value IN (
              'service', 'secondary', 'residential', 'trunk', 'primary',
              'unclassified', 'tertiary', 'living_street', 'motorway',
              'trunk_link', 'primary_link', 'motorway_link'
          )
        ORDER BY RAND()
        LIMIT 1;
    )");

    if (!query.exec() || !query.next()) {
        qDebug() << "Failed to retrieve random drivable way:" << query.lastError().text();
        return QString(); // Return empty if query fails
    }

    return query.value(0).toString(); // Return the first result
}

QMap<QString, QVector<QString>> DatabaseManager::buildNodesAdjacencyList() {
    QMap<QString, QVector<QString>> adjacencyList;

    QSqlQuery wayQuery;
    // Query to fetch all drivable ways
    wayQuery.prepare(R"(
        SELECT DISTINCT ways.id
        FROM ways
        JOIN tags ON ways.id = tags.element_id
        WHERE tags.tag_key = 'highway'
          AND tags.value IN (
              'service', 'secondary', 'residential', 'trunk', 'primary',
              'unclassified', 'tertiary', 'living_street', 'motorway',
              'trunk_link', 'primary_link', 'motorway_link'
          )
    )");

    if (!wayQuery.exec()) {
        qDebug() << "Failed to fetch drivable ways:" << wayQuery.lastError().text();
        return adjacencyList;
    }

    // Iterate through each drivable way
    while (wayQuery.next()) {
        QString wayId = wayQuery.value(0).toString();

        // Get all nodes in this way, ordered by their sequence
        QSqlQuery nodeQuery;
        nodeQuery.prepare(R"(
            SELECT node_id
            FROM ways_nodes
            WHERE way_id = :wayId
            ORDER BY node_order
        )");
        nodeQuery.bindValue(":wayId", wayId);

        if (!nodeQuery.exec()) {
            qDebug() << "Failed to fetch nodes for way" << wayId << ":" << nodeQuery.lastError().text();
            continue;
        }

        QVector<QString> nodes;
        while (nodeQuery.next()) {
            nodes.append(nodeQuery.value(0).toString());
        }

        // Connect nodes in sequence
        for (int i = 0; i < nodes.size() - 1; ++i) {
            const QString &currentNode = nodes[i];
            const QString &nextNode = nodes[i + 1];

            adjacencyList[currentNode].append(nextNode); // Connect current to next
            adjacencyList[nextNode].append(currentNode); // Connect next to current
        }
    }

    // Remove duplicate connections
    for (auto it = adjacencyList.begin(); it != adjacencyList.end(); ++it) {
        it.value().erase(std::unique(it.value().begin(), it.value().end()), it.value().end());
    }

    return adjacencyList;
}
QVector<QString> DatabaseManager::findPath(const QString &startNode, const QString &endNode, const QMap<QString, QVector<QString>> &adjacencyList) {
    if (!adjacencyList.contains(startNode) || !adjacencyList.contains(endNode)) {
        return {}; // Start or end node not in the adjacency list
    }

    QQueue<QString> queue;
    QMap<QString, QString> cameFrom; // Track the path
    queue.enqueue(startNode);
    cameFrom[startNode] = ""; // Start node has no predecessor

    while (!queue.isEmpty()) {
        QString current = queue.dequeue();

        if (current == endNode) {
            // Path found, reconstruct it
            QVector<QString> path;
            for (QString at = endNode; !at.isEmpty(); at = cameFrom[at]) {
                path.prepend(at);
            }
            return path;
        }

        for (const QString &neighbor : adjacencyList[current]) {
            if (!cameFrom.contains(neighbor)) {
                queue.enqueue(neighbor);
                cameFrom[neighbor] = current;
            }
        }
    }

    return {}; // No path found
}
void DatabaseManager::assignPathToCar(Car *car, const QMap<QString, QVector<QString>> &adjacencyList) {
    while (true) {
        // Choose random start and end nodes
        QStringList nodes = adjacencyList.keys();
        QString startNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];
        QString endNode = nodes[QRandomGenerator::global()->bounded(nodes.size())];

        if (startNode == endNode) {
            continue; // Avoid assigning the same node for start and end
        }

        QVector<QString> path = findPath(startNode, endNode, adjacencyList);

        if (!path.isEmpty()) {
            QVector<QPointF> positions;
            for (const QString &nodeId : path) {
                positions.append(getPositionByNodeId(nodeId));
            }

            car->setPath(positions, path); // Assign path to car
            qDebug() << "Assigned path to car" << car->getId() << "from" << startNode << "to" << endNode;
            return;
        }

        // If no path found, retry with new nodes
        qDebug() << "No path found from" << startNode << "to" << endNode << ", retrying...";
    }
}
