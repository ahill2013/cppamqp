/**
 * Florida Tech's IGVC program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 * Florida Tech's IGVC program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/linearizer.h"
#include "iostream"
/**
 * Library of functions for producing various approximations of the distance between two points
 */

namespace linearizer {
    /**
     * @brief Haversine function used to calculate the distance between two points in a perfect sphere
     * @param x_1 radius to a point
     * @param x_2 radius to a point
     * @return
     */
    double hav(double x_1, double x_2) {
        return pow(sin(x_2 - x_1), 2.0);
    }

    /**
     * @brief Vincenty calculation equation for meters to a degree given a latitude
     * @details As cited from Wikipedia but does not give accurate readings currently
     * @param latMid latitude to calculate from
     * @return meters to a degree given the latitude
     */
    double conversionToLat(double latMid) {
        return EQUATOR_MET_TO_DEG_LAT - 559.82 * cos(2.0 * latMid) + 1.175 * cos(4.0 * latMid) -
               0.0023 * cos(6.0 * latMid);
    }

    /**
     * @brief Given a latitude calculate the approximate meters a degree longitude at that latitude
     * @param latMid latitude to calculate conversion for
     * @return meters to a degree
     */
    double conversionToLon(double latMid) {
        return EQUATOR_MET_TO_DEG_LON - 93.5 * cos(3.0 * latMid) + 0.118 * cos(5.0 * latMid);
    }

    /**
     * @brief Given two gps coordinates calculate the vincenty approximation of the distance between those points
     * @warning Should be able to improve this approximation
     * @param coord1 first coordinate (from) must contain lat and lon at minimum
     * @param coord2 second coordinate (to) must contain lat and lon at minimum
     * @return distance between two gps points in meters
     */
    double vincenty(GPSMessage &coord1, GPSMessage &coord2) {
        double latMid = (coord1.lat + coord2.lat) / 2.0;
        double metersADegreeLat = conversionToLat(latMid);
        double metersADegreeLon = EQUATOR_MET_TO_DEG_LAT * cos(TORADS * latMid);

        double deltaLat = fabs(coord1.lat - coord2.lat);
        double deltaLon = fabs(coord1.lon - coord2.lon);

        return sqrt(pow(deltaLat * metersADegreeLat, 2.0) + pow(deltaLon * metersADegreeLon, 2.0));
    }

    /**
     * @brief Haversine distance approximation assuming earth is a perfect sphere. Sufficient for calculating distances
     * @brief between two points close together.
     * @param coord1 first gps coordinate
     * @param coord2 second gps coordinate
     * @return approximated distance in meters
     */
    double haversine(GPSMessage &coord1, GPSMessage &coord2) {
        double phi_1 = TORADS * coord1.lat,
                phi_2 = TORADS * coord2.lat;
        double lambda_1 = TORADS * coord1.lon;
        double lambda_2 = TORADS * coord2.lon;
        double d =
                2 * EARTHRADIUS * asin(sqrt(hav(phi_1, phi_2) + cos(phi_1) * cos(phi_2) * hav(lambda_1, lambda_2)));
        return 1000 * d;
    }

    /**
     * @brief Take a list of gps coordinates, cluster to remove a predetermined number of outliers, and average the
     * remaining points
     * @param recordedPositions list of pointers to gps messages, be careful to delete these pointers as necessary
     * @return averaged lat and lon of the list
     */
    GPSMessage* average(std::deque<GPSMessage*>& recordedPositions) {

        // Distance of the point furthest from the average lat and lon given by the list of GPSMessage
        double distance;

        // Indices of the outliers to remove from average calculation. Currently maximum to remove is two
        // Do not delete from the list of messages because you are deleting from something references in other code
        std::vector<int> outlierIndices;

        do {
            // Set this to zero if no point is more than DIST_DEF_OUTLIER meters away then there is no outlier
            distance = 0;

            // If we already have extracted the maximum outliers ...
            if (outlierIndices.size() >= MAX_OUTLIERS) {
                break;
            }

            // Average the list of coordinates minus outliers
            GPSMessage *average = average_iteration(recordedPositions, outlierIndices);

            int index = NO_INDEX;       // Index resultant from loop if no outliers are found, positive if outlier found
            int counter = 0;            // Counter records the index of the list

            // TODO: remove 'auto' keyword
            for (auto iter = recordedPositions.begin(); iter != recordedPositions.end(); ++iter) {
                if (contains(outlierIndices, counter)) {
                    counter++;
                    continue;
                }

                // Coordinate referred to by the iterator and get the distance from the average coordinate
                GPSMessage *pastLocation = *iter;
                double dist = vincenty(*average, *pastLocation);

                // If the distance from the average is the greatest distance seen in the list, save index as potential outlier
                if (distance < dist) {
                    distance = dist;
                    index = counter;
                }
                counter++;
            }

            // If outlier is recorded and distance from average is great enough
            // save index as an outlier
            if (index >= 0 && distance > DIST_DEF_OUTLIER) {
                outlierIndices.push_back(index);
            }

            printf("Distance %f\t\tIndex: %d\n", distance, index);
            delete average;
        } while (distance > DIST_DEF_OUTLIER);

        std::cout << "OUTLIERS FOUND\t" << outlierIndices.size() << std::endl;

        GPSMessage* message = average_iteration(recordedPositions, outlierIndices);
        printf("Found lat: %f, Found lon: %f\n", message->lat, message->lon);
        return message;
    }

    /**
     * @brief average a list of gps positions based on their lat and lon, while excluding indices of that list marked
     * @brief as outliers
     * @param recordedPositions list of past gps locations
     * @param outlierIndices list of indices that have been determined to be outliers
     * @return average lat and lon
     */
    GPSMessage* average_iteration(std::deque<GPSMessage*>& recordedPositions, std::vector<int>& outlierIndices) {
        // Get average position as of now
        double averageLat = 0;
        double averageLon = 0;

        // Figure out how many outliers there are
        double counter = 0;
        for (auto iter = recordedPositions.begin(); iter != recordedPositions.end(); ++iter) {

            // If outlier, then ignore point and move on to next location
            if (contains(outlierIndices, counter)) {
                printf("Continuing\n");
                counter++;
                continue;
            }

            // If not outlier add to averages
            GPSMessage *pastLocation = *iter;
            averageLat += pastLocation->lat;
            averageLon += pastLocation->lon;
            counter++;
        }

        // Divide summed lat and lon values by the number on non-outlier locations used
        averageLat /= (recordedPositions.size() - outlierIndices.size());
        averageLon /= (recordedPositions.size() - outlierIndices.size());

        // Return the average
        return new GPSMessage(averageLat, averageLon, 0, 0, 0, 0);
    }

    /**
     * Check whether a vector of integers contains an integer
     * @param outlierIndices vector of integers
     * @param at integer to look for
     * @return true if found
     */
    bool contains(std::vector<int>& outlierIndices, int at) {
        for (auto iter = outlierIndices.begin(); iter != outlierIndices.end(); iter++) {
            int index = *iter;
            if (index == at) {
                return true;
            }
        }

        return false;
    }
};

