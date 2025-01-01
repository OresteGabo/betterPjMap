#!/bin/bash

# Update your system
echo "Mise à jour du système"
sudo apt update && sudo apt upgrade -y

# Install required packages
echo "Installation des paquets nécessaires: build-essential, cmake, qt5-default"
sudo apt install build-essential cmake qt5-default -y

# Install MySQL server
echo "Installez MySQL Server"
sudo apt-get install mysql-server

# Install prerequisites for QMYSQL driver
echo "Installation des prérequis pour le driver QMYSQL"
sudo apt install libqt5sql5-mysql libmysqlclient-dev -y

# Navigate to your project directory
cd /home/oreste/CLionProjects/GraphicsInQt

# Create build directory and navigate into it
mkdir build && cd build

# Run CMake to configure the project and generate a build system
cmake ..

# Compile the project
make