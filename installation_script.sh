#!/bin/bash

# Function to install packages on Ubuntu
install_ubuntu() {
    echo "Updating system..."
    sudo apt update && sudo apt upgrade -y

    echo "Installing required packages: build-essential, cmake, qt5-default..."
    sudo apt install build-essential cmake qt5-default -y

    echo "Installing MySQL Server..."
    sudo apt-get install mysql-server

    echo "Installing prerequisites for QMYSQL driver..."
    sudo apt install libqt5sql5-mysql libmysqlclient-dev -y
}

# Function to install packages on macOS
install_mac() {
    echo "Installing Homebrew if not already installed..."
    if ! command -v brew &> /dev/null; then
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    echo "Installing required packages: cmake, qt, mysql..."
    brew install cmake qt mysql

    echo "Installing QMYSQL driver..."
    brew install qt-mysql
}

# Detect the operating system and run the appropriate installation function
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    install_ubuntu
elif [[ "$OSTYPE" == "darwin"* ]]; then
    install_mac
else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

# Navigate to your project directory
cd /path/to/your/project/directory

# Create build directory and navigate into it
mkdir build && cd build

# Run CMake to configure the project and generate a build system
cmake ..

# Compile the project
make
