cmake_minimum_required(VERSION 3.28)
project(GraphicsInQt)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt5 Components
find_package(Qt5 COMPONENTS
        Core
        Sql
        Gui
        Widgets
        REQUIRED)



# Add executable and source files
add_executable(GraphicsInQt
        main.cpp
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

# Include directories if necessary
include_directories(${CMAKE_SOURCE_DIR})

# Link libraries
target_link_libraries(GraphicsInQt
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        Qt5::Sql

)
