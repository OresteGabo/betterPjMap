#include <QApplication>
#include <QtSql/QSqlDatabase>
#include <QScreen>
#include "CustomScene.h"
#include "ConfigManager.h"
#include "CustomGraphicsView.h"
#include "DatabaseManager.h"
#include "MainWidget.h"
#include <iostream>

using namespace std;



int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Initialize database manager
    DatabaseManager dbMngr("OSMData", "oreste", "Muhirehonore@1*", "map.osm");

    // Create ConfigManager for config.json
    ConfigManager configManager("config.json");

    // Get screen size and configure main window dimensions
    auto screen = QGuiApplication::primaryScreen();
    auto screenGeometry = screen->geometry();
    configManager.setMainWindowSize(screenGeometry);


    // Create the main widget, passing in the custom scene and setting it up
    auto mainWidget = new MainWidget();
    //qDebug()<<"Available drivers are "<<QSqlDatabase::drivers();

    mainWidget->show();

    return app.exec();
}
