# 🚗 Connected Cars Simulation on OpenStreetMap (Qt6)

A high-performance C++ simulation built with **Qt 6** that parses OpenStreetMap (OSM) data to simulate an urban environment where connected vehicles interact in real-time.

---

## 📸 Project Gallery

|               City Infrastructure               | Connected Vehicle Logic |
|:-----------------------------------------------:| :---: |
| ![City View](cmake-build-debug/images/city.png) | ![Connected Cars](cmake-build-debug/images//connected_cars.png) |
|        *Rendering of OSM ways and nodes*        | *V2V connection via frequency proximity* |

---

## 🚀 The Engineering Journey (Optimization)

The core challenge of this project was rendering complex geographic data while maintaining a high frame rate for car movements.

### 1. The Bottleneck: QWidget & PaintEvents
Initially, I used a standard `QWidget` and overloaded the `paintEvent`. This proved inefficient because any movement required a full screen refresh, causing significant lag even with small `.osm` files.

### 2. Data Strategy: MySQL Integration
Parsing large XML files on every launch was slow. I implemented a **Database-First** approach:
* **Parsing:** `.osm` files (Nodes, Ways, Tags) are parsed once and stored in a **MySQL** database.
* **Retrieval:** The app queries `QSql` tables for faster data access during rendering.

### 3. Rendering: QGraphicsView Framework
To optimize the UI, I migrated to the `QGraphicsView` and `QGraphicsScene` framework. Unlike the paint event, this framework only redraws **modified elements** (the moving cars), leaving the static city map cached in the background.

### 4. Concurrency: Multithreaded Drawing
To display an entire city without freezing the UI, I utilized **Qt Concurrency**. Roads (Ways) are drawn independently in parallel, using `QApplication::processEvents()` to keep the interface responsive during the initial load.

---

## 🛠️ Core Features

* **V2V Connectivity:** Each car is assigned a random frequency. A visual circle represents the signal range; when two cars' circles overlap, they are marked as "Connected."
* **Geospatial Mapping:** A `BoundsCalculator` translates real-world Latitude and Longitude into optimized screen coordinates.
* **Network Grids:** A hexagonal grid system (`Maille`) is overlaid to represent network coverage areas.
* **Traffic Logic:** Cars follow specific road "Ways" and respect speed limit tags extracted from the OSM data.

---

## 📂 Class Architecture

| Class | Description |
| :--- | :--- |
| **`ConfigManager`** | Manages screen scaling and window dimensions. |
| **`Car`** | Logic for movement, speed limits, and connectivity range. |
| **`DatabaseManager`** | Handles MySQL connections and initial OSM XML parsing. |
| **`CustomScene/View`** | Custom rendering engine for the optimized GraphicsView. |
| **`BoundsCalculator`** | Normalizes GPS coordinates to local pixel coordinates. |
| **`Maille`** | Generates the hexagonal network grid logic. |
| **`Node` / `Way`** | Object representations of OpenStreetMap elements. |

---

## 🏗️ Build Instructions

### Prerequisites
* **Qt 6** (Core, Sql, Gui, Widgets)
* **MySQL Server**
* **CMake 3.28+**

### Compilation
```bash
mkdir build && cd build
cmake ..
make
./GraphicsInQt