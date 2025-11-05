#include <iostream>
#include <math.h>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <string>
#include <numeric>
#include <list>
#include <queue>
#include <unordered_map>


#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

#include "m2.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"

#include "m3.h"

// Global Variables
struct point_data{
    ezgl::point2d pos;
    std::string name;
    bool highlight = false;
};

extern double avg_lat;
extern double max_lat, min_lat, max_lon, min_lon;

extern std::vector<point_data> intersection_data;
extern std::vector<point_data> POI_data;
extern std::vector<std::pair<point_data, point_data>> segment_intersections;
extern std::vector<std::vector<StreetSegmentIdx>> street_segments_of_street; 
extern std::vector<gboolean> POI_on;
extern std::vector<StreetSegmentIdx> nav_segments;
extern std::vector<std::pair<double, std::vector<ezgl::point2d>>> feature_data;

// Global Helper Functions
std::string toLowerRemoveSpace(std::string input_string);

std::string getOSMWayTagValue(OSMID osm_id, std::string key);

double x_from_lon (float lon);

double y_from_lat (float lat);

double lat_from_y(float y);

double lon_from_x(float x);
