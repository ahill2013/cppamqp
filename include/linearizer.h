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

#ifndef VECTORNAV_LINEARIZER_H
#define VECTORNAV_LINEARIZER_H
#include "processor.h"
#include <math.h>
#include <deque>
#include <vector>

/**
 * @brief Functions to get distance between GPS points and improve positions
 * using simple clustering and averaging
 */
namespace linearizer {
    double EARTHRADIUS = 6378.137;      // Earth's radius in km according to WGS standard
    double TORADS = M_PI / 180.0;

    double EQUATOR_MET_TO_DEG_LAT = 111132.954; // Meters a degree latitude at the equator
    double EQUATOR_MET_TO_DEG_LON = 111412.84;  // Meters a degree longitude at the equator

    int MAX_OUTLIERS = 2;
    int DIST_DEF_OUTLIER = 1.5;
    int NO_INDEX = -1;


    double hav(double, double);
    double conversionToLon(double lon);
    double conversionToLat(double lat);
    double vincenty(GPSMessage&, GPSMessage&);
    double haversine(GPSMessage&, GPSMessage&);
    GPSMessage* average(std::deque<GPSMessage*>&);
    GPSMessage* average_iteration(std::deque<GPSMessage*>&, std::vector<int>&);
    bool contains(std::vector<int>&, int);

}
#endif //VECTORNAV_LINEARIZER_H
