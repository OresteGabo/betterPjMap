#ifndef GRAPHICSINQT_CONFIGMANAGER_H
#define GRAPHICSINQT_CONFIGMANAGER_H

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRect>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
class ConfigManager {
    QJsonObject jsonObj;
    QString configFileName;

public:
    ConfigManager(const QString &configFileName = "config.json") : configFileName(configFileName) {
        QFile file(configFileName);
        // If the file doesn't exist, create it
        if (!file.exists()) {
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
            }
        }

        // Read file contents
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileData = file.readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(fileData);
            jsonObj = jsonDoc.isObject() ? jsonDoc.object() : QJsonObject();
            file.close();
        }

        // Get screen size and configure main window dimensions
        auto screen = QGuiApplication::primaryScreen();
        auto screenGeometry = screen->geometry();
        setMainWindowSize(screenGeometry);
    }



    void setMainWindowSize(const QRect &screenGeometry) {
        QJsonObject mainWindowObj;
        mainWindowObj["width"] = screenGeometry.width();
        mainWindowObj["height"] = screenGeometry.height();
        qDebug() << "mainWindowObj[\"width\"]:" << mainWindowObj["width"].toInt()
                 << ", mainWindowObj[\"height\"]:" << mainWindowObj["height"].toInt();
        jsonObj["MainWindow"] = mainWindowObj;

        writeToFile();
    }

    QRect getMainWindowSize() {
        if (jsonObj.contains("MainWindow")) {
            QJsonObject mainWindowObj = jsonObj["MainWindow"].toObject();
            int width = mainWindowObj["width"].toInt();
            int height = mainWindowObj["height"].toInt();
            return {0, 0, width - 100, height - 50};
        }
        return {0, 0, 800, 600};  // Default size
    }

    void setBounds(double minLat, double maxLat, double minLon, double maxLon) {
        // Create a JSON object to hold bounds information
        QJsonObject boundsObj;
        boundsObj["minLat"] = minLat;
        boundsObj["maxLat"] = maxLat;
        boundsObj["minLon"] = minLon;
        boundsObj["maxLon"] = maxLon;

        // Add the bounds object to the main JSON structure
        jsonObj["Bound"] = boundsObj;

        // Write the updated JSON to file
        writeToFile();
    }

    static QJsonObject loadJsonFile(const QString &configFileName="config.json") {
        QFile file(configFileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Could not open JSON file:" << configFileName;
            return QJsonObject();
        }
        QByteArray jsonData = file.readAll();
        QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));
        return  jsonDoc.object();
    }


private:
    void writeToFile() {
        QFile file("config.json");
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Could not open config file for writing";
            return;
        }
        QJsonDocument doc(jsonObj);
        file.write(doc.toJson());
        file.close();
    }





};

#endif // GRAPHICSINQT_CONFIGMANAGER_H
