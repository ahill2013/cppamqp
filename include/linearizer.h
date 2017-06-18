//
// Created by igvc-spec2 on 6/1/17.
//

#ifndef VECTORNAV_LINEARIZER_H
#define VECTORNAV_LINEARIZER_H
#include <myamqp/processor.h>
#include <math.h>
#include <deque>
#include <vector>

namespace linearizer {
    double EARTHRADIUS = 6378.137;      // Earth's radius in km according to WGS standard
    double TORADS = M_PI / 180.0;

    double EQUATOR_MET_TO_DEG_LAT = 111132.954;
    double EQUATOR_MET_TO_DEG_LON = 111412.84;

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
