#ifndef DATABASE_MANAGER
#define DATABASE_MANAGER
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMap>
#include "ConfigManager.h"
#include <QDebug>
#include "CustomScene.h"
// Adjacency list type: a map where each node ID maps to a set of connected node IDs
using AdjacencyList = QMap<QString, QVector<QString>>;
class DatabaseManager {
public:

    DatabaseManager(const QString& dbName,
                    const QString& user,
                    const QString& password,
                    const QString& fileName);
    ~DatabaseManager()
    {

        // close database connection if necessary
    }

    bool parseData(const QString &fileName);

    // Get the minimum and maximum latitude and longitude from a query
    void getBounds(QSqlQuery &query, double &minLat, double &maxLat, double &minLon, double &maxLon) {
        while (query.next()) {
            double lat = query.value(0).toDouble();
            double lon = query.value(1).toDouble();

            if (lat < minLat) minLat = lat;
            if (lat > maxLat) maxLat = lat;
            if (lon < minLon) minLon = lon;
            if (lon > maxLon) maxLon = lon;
        }

    }


    bool calculateAndSaveBoundsToConfig(ConfigManager &configManager) {
        QSqlQuery query;
        auto minLat = 90.0, maxLat = -90.0, minLon = 180.0, maxLon = -180.0;

        // Calculate bounds by querying node coordinates
        query.exec("SELECT lat, lon FROM nodes");
        getBounds(query, minLat, maxLat, minLon, maxLon);

        // Debug statement to verify bounds
        qDebug() << "Bounds calculated - MinLat:" << minLat << ", MaxLat:" << maxLat
                 << ", MinLon:" << minLon << ", MaxLon:" << maxLon;

        // Set the bounds in ConfigManager and ensure they are saved
        configManager.setBounds(minLat, maxLat, minLon, maxLon);

        // Check if configManager successfully saved bounds
        qDebug() << "Bounds have been set in ConfigManager";

        return true;
    }
    static AdjacencyList buildAdjacencyList();
    static AdjacencyList buildDrivableAdjacencyList();



    static QVector<QString> getDrivableWaysIds();
    static QString getWayNameById(const QString& wayId);
    static QVector<QString> getNodesOfWay(const QString& wayId);
    static QString getRandomDrivableWay();
    static QVector<QString> getNodesOfWaysWithName();
    static QPointF getPositionByNodeId(QString id);
    //static QString getStreetNameByNode(const QString& nodeId);
    static QString getWayNameForNode(const QString &nodeId);
    static QVector<QString> getDrivableWaysNodesId(){
        QSet<QString> nodesId;
        for(auto wayId: getDrivableWaysIds()){
            auto nodes= getNodesOfWay(wayId);
            for(auto node:nodes){
                nodesId.insert(node);
            }
        }
        QVector<QString> result{};
        for(auto nodeId:nodesId){
            result.push_back(nodeId);
        }
        return result;
    }


    QSqlDatabase db;

    void initialiseDatabase(const QString &dbName,
                            const QString &user,
                            const QString &password);

    static bool parseNodes(QXmlStreamReader &xml, QSqlQuery &query);
    static bool parseWays(QXmlStreamReader &xml, QSqlQuery &query);

    //select distinct element_id from tags where element_type ='node' and tag_key='amenity' and value='fountain';
    //static QVector<QString> getAllAmenitiesByType(const QString& value);
    void markJunctionNodes();
    QMap<QString, QString> getWayTags(const QString &wayId);
    static QMap<QString, QVector<QString>> buildNodesAdjacencyList();
    static QVector<QString> findPath(const QString &startNode, const QString &endNode, const QMap<QString, QVector<QString>> &adjacencyList);
    static void assignPathToCar(Car *car, const QMap<QString, QVector<QString>> &adjacencyList);

    static QString findNextWay(const QString &lastNode);

private:

    void truncateAllTables() {
        QSqlQuery query;
        QStringList tables = {"relations_members","ways_nodes", "tags","relations","ways","address","nodes"};

        db.transaction();
        for (const auto &tableName : tables) {
            if(query.exec("DROP TABLE " + tableName)){
                qDebug()<<"Table dropped "<<tableName;
            }
        }
        db.commit();
    }
    void createTables() ;

};

#endif