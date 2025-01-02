cmake_minimum_required(VERSION 3.28)
project(GraphicsInQt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt5 COMPONENTS
        Core
        Sql
        Gui
        Widgets
        REQUIRED)

add_executable(GraphicsInQt main.cpp
        Node.cpp
        Node.h
        CustomScene.cpp
        CustomScene.h
        Way.cpp
        Way.h
        DatabaseManager.cpp
        DatabaseManager.h
        ConfigManager.cpp
        ConfigManager.h
        CustomGraphicsView.cpp
        CustomGraphicsView.h
        Car.cpp
        Car.h
        MainWidget.cpp
        MainWidget.h
        Maille.cpp
        Maille.h
)
target_link_libraries(GraphicsInQt
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql
)

