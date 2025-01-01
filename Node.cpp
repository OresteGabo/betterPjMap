//
// Created by oreste on 28/10/24.
//

#include "Node.h"
#include <QBrush>
#include <QtMath>


// Haversine formula for distance calculation
double Node::distanceTo(const Node *other) const {
    const double R = 6371e3; // Earth radius in meters
    double lat1 = qDegreesToRadians(lat);
    double lat2 = qDegreesToRadians(other->lat);
    double deltaLat = qDegreesToRadians(other->lat - lat);
    double deltaLon = qDegreesToRadians(other->lon - lon);

    double a = sin(deltaLat / 2) * sin(deltaLat / 2) +
               cos(lat1) * cos(lat2) * sin(deltaLon / 2) * sin(deltaLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return R * c;
}
