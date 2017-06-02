
#include "../include/linearizer.h"
#include "iostream"
/**
 * Library of functions for producing various approximations of the distance between two points
 */

namespace linearizer {
// Haversine function
    double hav(double x_1, double x_2) {
        return pow(sin(x_2 - x_1), 2.0);
    }

// For vincenty approximation
// Meters to a degree of latitude for a given latitude
    double conversionToLat(double latMid) {
        return EQUATOR_MET_TO_DEG_LAT - 559.82 * cos(2.0 * latMid) + 1.175 * cos(4.0 * latMid) -
               0.0023 * cos(6.0 * latMid);
    }

// For vincenty approximation
// Meters to a degree of longitude for a given longitude
    double conversionToLon(double latMid) {
        return EQUATOR_MET_TO_DEG_LON - 93.5 * cos(3.0 * latMid) + 0.118 * cos(5.0 * latMid);
    }

// Return distance in meters between two coordinates
// uses part of the vincenty calculation
    double vincenty(GPSMessage &coord1, GPSMessage &coord2) {
        double latMid = (coord1.lat + coord2.lat) / 2.0;
        double metersADegreeLat = conversionToLat(latMid);
        double metersADegreeLon = EQUATOR_MET_TO_DEG_LAT * cos(TORADS * latMid);

        double deltaLat = fabs(coord1.lat - coord2.lat);
        double deltaLon = fabs(coord1.lon - coord2.lon);

        return sqrt(pow(deltaLat * metersADegreeLat, 2.0) + pow(deltaLon * metersADegreeLon, 2.0));
    }

// Return distance in meters between two coordinates
// assumes haversine
    double haversine(GPSMessage &coord1, GPSMessage &coord2) {
        double phi_1 = TORADS * coord1.lat,
                phi_2 = TORADS * coord2.lat;
        double lambda_1 = TORADS * coord1.lon;
        double lambda_2 = TORADS * coord2.lon;
        double d =
                2 * EARTHRADIUS * asin(sqrt(hav(phi_1, phi_2) + cos(phi_1) * cos(phi_2) * hav(lambda_1, lambda_2)));
        return 1000 * d;
    }

    GPSMessage* average(std::deque<GPSMessage*>& recordedPositions) {

        // Distance of the point furthest from the average lat and lon given by the list of GPSMessage
        double distance;

        // Indices of the outliers to remove currently maximum to remove is two
        std::vector<int> indices;

        do {
            // Set this to zero if no point is more than DIST_DEF_OUTLIER meters away then there is no outlier
            distance = 0;

            // If we already have extracted the maximum outliers ...
            if (indices.size() >= MAX_OUTLIERS) {
                break;
            }

            GPSMessage *average = average_iteration(recordedPositions, indices);

            int index = NO_INDEX;
            int counter = 0;
            for (auto iter = recordedPositions.begin(); iter != recordedPositions.end(); ++iter) {
                if (contains(indices, counter)) {
                    counter++;
                    continue;
                }
                GPSMessage *pastLocation = *iter;
                double dist = vincenty(*average, *pastLocation);

                if (distance < dist) {
                    distance = dist;
                    index = counter;
                }
                counter++;
            }

            if (index >= 0 && distance > DIST_DEF_OUTLIER) {
                indices.push_back(index);
            }

            printf("Distance %f\t\tIndex: %d\n", distance, index);
            delete average;
        } while (distance > DIST_DEF_OUTLIER);

        std::cout << "OUTLIERS FOUND\t" << indices.size() << std::endl;

        GPSMessage* message = average_iteration(recordedPositions, indices);
        printf("Found lat: %f, Found lon: %f\n", message->lat, message->lon);
        return message;
    }

    GPSMessage* average_iteration(std::deque<GPSMessage*>& recordedPositions, std::vector<int>& indices) {
        // Get average position as of now
        double averageLat = 0;
        double averageLon = 0;
        double counter = 0;
        for (auto iter = recordedPositions.begin(); iter != recordedPositions.end(); ++iter) {
            if (contains(indices, counter)) {
                printf("Continuing\n");
                counter++;
                continue;
            }
            GPSMessage *pastLocation = *iter;
            averageLat += pastLocation->lat;
            averageLon += pastLocation->lon;
            counter++;
        }

        averageLat /= (recordedPositions.size() - indices.size());
        averageLon /= (recordedPositions.size() - indices.size());

        return new GPSMessage(averageLat, averageLon, 0, 0, 0, 0);
    }

// Check if one of the indices is already a known outlier
    bool contains(std::vector<int>& indices, int at) {
        for (auto iter = indices.begin(); iter != indices.end(); iter++) {
            int index = *iter;
            if (index == at) {
                return true;
            }
        }

        return false;
    }
};

