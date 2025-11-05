
#include "globals.h"
#include <iostream>
#include <math.h>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <string>

#include "m1.h"
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"

//Helper Function Prototypes
const OSMNode* getNodeByID(OSMID osm_id);
const OSMWay* getWayByID(OSMID osm_id);
bool strReplace(std::string& source, const std::string& from_str, const std::string& to_str);
double cosLaw(double a, double b, double c);

// ------------- Samiyah's Functions -------------

// loadMap will be called with the name of the file that stores the "layer-2"
// map data accessed through StreetsDatabaseAPI: the street and intersection 
// data that is higher-level than the raw OSM data). 
// This file name will always end in ".streets.bin" and you 
// can call loadStreetsDatabaseBIN with this filename to initialize the
// layer 2 (StreetsDatabase) API.
// If you need data from the lower level, layer 1, API that provides raw OSM
// data (nodes, ways, etc.) you will also need to initialize the layer 1 
// OSMDatabaseAPI by calling loadOSMDatabaseBIN. That function needs the 
// name of the ".osm.bin" file that matches your map -- just change 
// ".streets" to ".osm" in the map_streets_database_filename to get the proper
// name.

// Global variables
std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments; 
std::map<char, std::multimap<std::string, StreetIdx>> street_names; 
std::unordered_map<OSMID, const OSMNode*> OSM_node_ID; 
std::unordered_map<OSMID, std::unordered_map<std::string, std::string>> OSM_node_tags; 
std::unordered_map<OSMID, std::unordered_map<std::string, std::string>> OSM_way_tags; 
std::unordered_map<OSMID, double> OSM_way_length; 
std::unordered_map<OSMID, const OSMWay*> OSM_way_ID; 
std::vector<double> street_travel_time;
std::vector<double> street_lengths;
std::vector<std::vector<IntersectionIdx>> street_intersections;
std::vector<point_data> intersection_data;
std::vector<point_data> POI_data;
std::vector<std::pair<point_data, point_data>> segment_intersections;
std::vector<std::vector<StreetSegmentIdx>> street_segments_of_street; 
std::vector<std::pair<double, std::vector<ezgl::point2d>>> feature_data;

double avg_lat = 0;
double max_lat, min_lat, max_lon, min_lon = 0;

bool loadMap(std::string map_streets_database_filename) {
    //false, Indicates whether the map has loaded 
    bool load_successful = loadStreetsDatabaseBIN(map_streets_database_filename); 
    bool load_OSM = false;

    //Successfully loaded
    std::string from = ".streets.bin";
    std::string to = ".osm.bin";
    if(load_successful == true){
        // auto const cp1 = std::chrono::high_resolution_clock::now();

        // This following function call, strReplace, replaces the mapstreetdatadbasefilenanme from .string to .osm to use in osmload
        bool osmBinStringChange = strReplace(map_streets_database_filename, from, to);

        // Loads osm database for use of osm header file functions, like return nodes, ways,etc
        if (osmBinStringChange){
            load_OSM = loadOSMDatabaseBIN(map_streets_database_filename);
        }

        // auto const cp2 = std::chrono::high_resolution_clock::now();
        // auto const delta = std::chrono::duration_cast<std::chrono::milliseconds>(cp2 - cp1);
        // std::cout << "OSM load took " << delta.count() << " ms" << std::endl;

        // Preload various iteration bounds
        int num_intersections = getNumIntersections();
        int num_segments = getNumStreetSegments();
        int num_streets = getNumStreets();
        int num_nodes = getNumberOfNodes();
        int num_ways = getNumberOfWays();
        int num_poi = getNumPointsOfInterest();
        
        // Load vectors for multiple functions related to street segment
        intersection_street_segments.resize(num_intersections); 
        street_segments_of_street.resize(num_streets); 
        intersection_data.resize(num_intersections); 
        street_lengths.resize(num_streets);
        POI_data.resize(num_poi); 
        double length = 0;

        // Find avg_lat for m2 function
        max_lat = getIntersectionPosition(0).latitude(); 
        min_lat = max_lat;
        max_lon = getIntersectionPosition(0).longitude(); 
        min_lon = max_lon;
        
        for (int i = 0; i < num_intersections; i++){
            LatLon inter_pos = getIntersectionPosition(i);
            
            // Compare every lat/lon of intersection w initial to find max/min
            max_lat = std::max(max_lat, inter_pos.latitude());
            min_lat = std::min(min_lat, inter_pos.latitude());
            max_lon = std::max(max_lon, inter_pos.longitude());
            min_lon = std::min(min_lon, inter_pos.longitude());
        }
    
        avg_lat = ((min_lat + max_lat)/2);

        // auto const cp3 = std::chrono::high_resolution_clock::now();
        // auto delta2 = std::chrono::duration_cast<std::chrono::milliseconds>(cp3 - cp2);
        // std::cout << "Inits took " << delta2.count() << " ms" << std::endl;

        // Load intersection data
        // Note: the following loop cannot be merged with the one after street segments loop
        // Street segments loop needs this loop done first, but the one after that needs
        // The street segments loop done first
        for (IntersectionIdx i = 0; i < num_intersections; i++){
            intersection_data[i].name = getIntersectionName(i);
            LatLon inter_pos = getIntersectionPosition(i);
            intersection_data[i].pos.x = x_from_lon(inter_pos.longitude());
            intersection_data[i].pos.y = y_from_lat(inter_pos.latitude());
        }   

        // auto const cp4 = std::chrono::high_resolution_clock::now();
        // auto delta3 = std::chrono::duration_cast<std::chrono::milliseconds>(cp4 - cp3);
        // std::cout << "Intersections 1 took " << delta3.count() << " ms" << std::endl;

        // Load various Street Segment related data structures
        segment_intersections.resize(num_segments);
        street_intersections.resize(num_streets);
        point_data intersection_to, intersection_from;

        for (StreetSegmentIdx i = 0; i < num_segments; i++){
            StreetSegmentInfo street_info = getStreetSegmentInfo(i);
            
            //Intersections of street segment
            intersection_street_segments[street_info.to].push_back(i);
            intersection_street_segments[street_info.from].push_back(i);

            // Travel time
            length = findStreetSegmentLength(i);
            street_travel_time.push_back(length/street_info.speedLimit);

            // Street length
            street_lengths[street_info.streetID] += length;

            // Street segments of street
            street_segments_of_street[street_info.streetID].push_back(i);

            // Intersection xy coordinates
            intersection_to = intersection_data[street_info.to];
            intersection_from = intersection_data[street_info.from];

            segment_intersections[i] = std::make_pair(intersection_from, intersection_to);    
           
            street_intersections[street_info.streetID].push_back(street_info.to);
            street_intersections[street_info.streetID].push_back(street_info.from);
        } 

        for (int i = 0; i < street_intersections.size(); i++){
            std::sort(street_intersections[i].begin(), street_intersections[i].end());
            street_intersections[i].erase(std::unique(street_intersections[i].begin(), street_intersections[i].end()), street_intersections[i].end());
        }   

        // auto const cp5 = std::chrono::high_resolution_clock::now();
        // auto delta4 = std::chrono::duration_cast<std::chrono::milliseconds>(cp5 - cp4);
        // std::cout << "Segments took " << delta4.count() << " ms" << std::endl;

        // Load POI info
        for (POIIdx i = 0; i < num_poi; i++){
            LatLon POI_pos = getPOIPosition(i);
            POI_data[i].pos.x = x_from_lon(POI_pos.longitude());
            POI_data[i].pos.y = y_from_lat(POI_pos.latitude());
            POI_data[i].name = getPOIName(i);
        }

        // auto const cp7 = std::chrono::high_resolution_clock::now();
        // auto delta6 = std::chrono::duration_cast<std::chrono::milliseconds>(cp7 - cp5);
        // std::cout << "POIs took " << delta6.count() << " ms" << std::endl;

        // Load feature data
        for (int i = 0; i < getNumFeatures(); i++){
            std::vector<ezgl::point2d> feature_points;
            double area = findFeatureArea(i);
            int num_points = getNumFeaturePoints(i);
            
            feature_points.resize(num_points);

            for (int j = 0; j < num_points; j++){
                feature_points[j].x = x_from_lon(getFeaturePoint(j, i).longitude());
                feature_points[j].y = y_from_lat(getFeaturePoint(j, i).latitude()); 
            }

            feature_data.push_back(std::make_pair(area, feature_points));
        }

        // auto const cp8 = std::chrono::high_resolution_clock::now();
        // auto delta7 = std::chrono::duration_cast<std::chrono::milliseconds>(cp8 - cp7);
        // std::cout << "Features took " << delta7.count() << " ms" << std::endl;

        // Loading data structure street_names to use with partial street name func
        for (char c = 97; c < 123; c++){
            std::multimap<std::string, StreetIdx> letter;
            street_names.insert(std::make_pair(c, letter));
        }
        for (StreetIdx i = 0; i < num_streets; i++){
            //Turn street name string to lowercase w no spaces
            std::string currStreet = toLowerRemoveSpace(getStreetName(i));
            //Insert in alphabetical categorization
            char c = currStreet[0];
            street_names[c].insert(std::make_pair(currStreet, i));
        }

        // auto const cp9 = std::chrono::high_resolution_clock::now();
        // auto delta8 = std::chrono::duration_cast<std::chrono::milliseconds>(cp9 - cp8);
        // std::cout << "St names took " << delta8.count() << " ms" << std::endl;

        // Load data structures OSM_node_ID, OSM_node_tags and OSM_way_ID for use with OSM related functions
        const OSMNode *node_insert = NULL;
        std::unordered_map<std::string, std::string> tag_map;
        for (int i = 0; i < num_nodes; i++){
            node_insert = getNodeByIndex(i);
            if (node_insert != NULL){
                OSMID id = node_insert -> id();
                OSM_node_ID.insert(std::make_pair(id, node_insert));
                OSM_node_tags.insert(std::make_pair(id, tag_map));
                for(int j = 0; j < getTagCount(node_insert); j++){
                    std::pair tag = getTagPair(node_insert, j);
                    OSM_node_tags[id].insert(tag);
                }
            }
        }

        const OSMWay *way_insert = NULL;
        for (int i = 0; i < num_ways; i++){
            length = 0;
            way_insert = getWayByIndex(i);
            if (way_insert != NULL){
                OSM_way_ID.insert(std::make_pair(way_insert -> id(), way_insert));
                std::vector<OSMID> way_members = getWayMembers(way_insert);

                for (int j = 1; j < way_members.size(); j++){
                    auto node1 = OSM_node_ID[way_members[j-1]];
                    auto node2 = OSM_node_ID[way_members[j]];

                    LatLon p1 = getNodeCoords(node1);
                    LatLon p2 = getNodeCoords(node2);
                    length += findDistanceBetweenTwoPoints(p1, p2);
                }

                if (isClosedWay(way_insert)){
                    auto node1 = OSM_node_ID[way_members[0]];
                    auto node2 = OSM_node_ID[way_members[way_members.size() - 1]];

                    LatLon p1 = getNodeCoords(node1);
                    LatLon p2 = getNodeCoords(node2);
                    length += findDistanceBetweenTwoPoints(p1, p2);
                }

                OSM_way_tags.insert(std::make_pair(way_insert -> id(), tag_map));
                for(int j = 0; j < getTagCount(way_insert); j++){
                    std::pair tag = getTagPair(way_insert, j);
                    OSM_way_tags[way_insert -> id()].insert(tag);
                }
            }
            OSM_way_length.insert(std::make_pair(way_insert -> id(), length));
        }

        // auto const cp10 = std::chrono::high_resolution_clock::now();
        // auto delta9 = std::chrono::duration_cast<std::chrono::milliseconds>(cp10 - cp9);
        // std::cout << "OSM data took " << delta9.count() << " ms" << std::endl;
    }

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;

    return load_successful && load_OSM;
}

/* no dynamic memory allocation in m1.cpp, but api functions will deal with 
dynam array atd: vector, hence just closing the apis */
void closeMap() {
    closeStreetDatabase(); 
    closeOSMDatabase();
    // Clear data structure to avoid duplicates
    intersection_street_segments.clear(); 
    street_names.clear();
    OSM_node_ID.clear();
    OSM_node_tags.clear();
    OSM_way_tags.clear();
    OSM_way_length.clear();
    OSM_way_ID.clear();
    street_travel_time.clear();
    street_lengths.clear();
    street_intersections.clear();
    intersection_data.clear();
    POI_data.clear();
    segment_intersections.clear();
    street_segments_of_street.clear();
    POI_on.clear();
    nav_segments.clear();
    feature_data.clear();
}

// Returns all intersections along the given street.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id){
    return street_intersections[street_id];
}

// Return all intersection ids at which the two given streets intersect.
// This function will typically return one intersection id for streets that
// intersect and a length 0 vector for streets that do not. For unusual curved
// streets it is possible to have more than one intersection at which two
// streets cross.
// There should be no duplicate intersections in the returned vector.
// Speed Requirement --> high
std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(std::pair<StreetIdx, StreetIdx> street_ids){
    std::vector<IntersectionIdx> common_intersections;
    std::vector<IntersectionIdx> st_one = findIntersectionsOfStreet(street_ids.first);
    std::vector<IntersectionIdx> st_two = findIntersectionsOfStreet(street_ids.second);
    for (int i = 0; i < st_one.size(); i++){
        auto found = std::find(st_two.begin(), st_two.end(), st_one[i]);
        if (found != st_two.end()){
            common_intersections.push_back(st_one[i]);
        }
    }
    return common_intersections;
}

// Returns the nearest point of interest of the given name (e.g. "Starbucks")
// to the given position.
// Speed Requirement --> none 
POIIdx findClosestPOI(LatLon my_position, std::string poi_name){
//findDistanceBetweenTwoPoints(LatLon point_1, LatLon point_2)
// traverse through poi's

    double distance=-1;
    //pos 0 initial
    POIIdx fcPOI=0; 
  
    for (POIIdx p=0; p<getNumPointsOfInterest(); p++){
        
        //if poi has this name, check for position      
        if (poi_name.compare(getPOIName(p))==0){
            
            if (distance==-1){
            distance=findDistanceBetweenTwoPoints(my_position,getPOIPosition(p)); 
            fcPOI=p;
            }
            //fcPOI=100;
            if((findDistanceBetweenTwoPoints(my_position, getPOIPosition(p)))<distance){
            distance=findDistanceBetweenTwoPoints(my_position, getPOIPosition(p));
            fcPOI=p;
            }
        }
    }
   return fcPOI;  

}

// Returns the length of the given street in meters.
// Speed Requirement --> moderate
double findStreetLength(StreetIdx street_id){
    return street_lengths[street_id];
}

// ------------- Laurel's Functions -------------

// Returns the distance between two (lattitude,longitude) coordinates in meters.
// Speed Requirement --> moderate
double findDistanceBetweenTwoPoints(LatLon point_1, LatLon point_2){    
    // calculate average latitude
    double lat_Avg = (point_1.latitude() + point_2.latitude())/2;
    
    // calculate values of point coordinates
    double x1 = kEarthRadiusInMeters * kDegreeToRadian * point_1.longitude() * cos(kDegreeToRadian*lat_Avg); 
    double x2 = kEarthRadiusInMeters * kDegreeToRadian * point_2.longitude() * cos(kDegreeToRadian*lat_Avg); 
    double y1 = kEarthRadiusInMeters * kDegreeToRadian * point_1.latitude(); 
    double y2 = kEarthRadiusInMeters * kDegreeToRadian * point_2.latitude();
    
    //calculate distance using distance between two points formula
    double distance = sqrt(((y2-y1)*(y2-y1)) + ((x2-x1)*(x2-x1))); 
    //return distance
    return distance; 
}

// Returns the street segments that connect to the given intersection.
// Speed Requirement --> high
std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(IntersectionIdx intersection_id){
    // Intersection not found in the map, return an empty vector or handle accordingly
    return intersection_street_segments[intersection_id];
}

// Returns true if the two intersections are directly connected, meaning you can
// legally drive from the first intersection to the second using only one
// streetSegment.
// Speed Requirement --> moderate
bool intersectionsAreDirectlyConnected(std::pair<IntersectionIdx, IntersectionIdx> intersection_ids){
    IntersectionIdx intersection1, intersection2, segment_from, segment_to;
    
    intersection1 = intersection_ids.first;
    intersection2 = intersection_ids.second;
    
    std::vector<StreetSegmentIdx> vector1 = findStreetSegmentsOfIntersection(intersection1);
    std::vector<StreetSegmentIdx> vector2 = findStreetSegmentsOfIntersection(intersection2);
    
    // go through intersection vectors to see if there are any matching segments
    for(auto vector1_element_first_it = vector1.begin(); vector1_element_first_it < vector1.end(); vector1_element_first_it++){      
       segment_from = getStreetSegmentInfo(*vector1_element_first_it).from;
       segment_to = getStreetSegmentInfo(*vector1_element_first_it).to;
       
       if((intersection1 == intersection2) || ((intersection1 == segment_from) && (intersection2 == segment_to)) || ((intersection1 == segment_to) && (intersection2 == segment_from))){
           return true;
       }
    }
    return false;
    
} 

// Returns the geographically nearest intersection (i.e. as the crow flies) to
// the given position.
// Speed Requirement --> none
IntersectionIdx findClosestIntersection(LatLon my_position){

    double shortest_distance = findDistanceBetweenTwoPoints(my_position, getIntersectionPosition(0));
    IntersectionIdx closestintersection = 0;
    
    // looping through intersections and updating shortest distance as new closest intersection 
    for (int intersection = 0; intersection < getNumIntersections(); ++intersection) { // loop of each intersection
            double checkdistance = findDistanceBetweenTwoPoints(my_position,getIntersectionPosition(intersection));
            if (checkdistance < shortest_distance){
                shortest_distance = checkdistance;
                closestintersection = intersection;
            }
        }   
    return closestintersection;
  
}

// Returns the length of the given street segment in meters.
// Speed Requirement --> moderate

double findStreetSegmentLength(StreetSegmentIdx street_segment_id){
   StreetSegmentInfo segment_info = getStreetSegmentInfo(street_segment_id);
   int num_curve_points = segment_info.numCurvePoints;
   double curve_length = 0, street_segment_length = 0, start_curve, end_curve;
   
   LatLon intersection_1 = getIntersectionPosition(segment_info.from);
   LatLon intersection_2 = getIntersectionPosition(segment_info.to);
   
   // calculating length of a street segment with curve points
   if(num_curve_points > 0){
        LatLon first_curve_point = getStreetSegmentCurvePoint(0, street_segment_id);
        LatLon last_curve_point = getStreetSegmentCurvePoint(num_curve_points - 1, street_segment_id);
        
        for (int i = 0; i < num_curve_points - 1; i++) {
            LatLon p1 = getStreetSegmentCurvePoint(i, street_segment_id);
            LatLon p2 = getStreetSegmentCurvePoint(i + 1, street_segment_id);

            curve_length += findDistanceBetweenTwoPoints(p1, p2);
        }
      
        start_curve = findDistanceBetweenTwoPoints(intersection_1, first_curve_point);
        end_curve = findDistanceBetweenTwoPoints(last_curve_point, intersection_2);

        street_segment_length = curve_length + start_curve + end_curve;
   }
   else{
       // calculating length of a street segment without curve points
       street_segment_length = findDistanceBetweenTwoPoints(intersection_1, intersection_2);
   }
   
   return street_segment_length;
}



// ------------- Nusaiba's Functions -------------

// Returns the angle (in radians) that would result as you exit
// src_street_segment_id and enter dst_street_segment_id, if they share an
// intersection.
// If a street segment is not completely straight, use the last piece of the
// segment closest to the shared intersection.
// If the two street segments do not share an intersection, return a constant
// NO_ANGLE, which is defined above.
// Speed Requirement --> none
double findAngleBetweenStreetSegments(StreetSegmentIdx src_street_segment_id, StreetSegmentIdx dst_street_segment_id){
    StreetSegmentInfo st_One = getStreetSegmentInfo(src_street_segment_id);
    StreetSegmentInfo st_Two = getStreetSegmentInfo(dst_street_segment_id);

    LatLon p1;
    LatLon p2;
    LatLon p3;

    // Determine the intersection point and the two non intersection points. convert to latlon points

    if (st_One.to == st_Two.to){
        p1 = getIntersectionPosition(st_One.to);
            if (st_One.numCurvePoints > 0){
                p2 = getStreetSegmentCurvePoint(st_One.numCurvePoints - 1, src_street_segment_id);
            }
            else{
                p2 = getIntersectionPosition(st_One.from);
            }
            if (st_Two.numCurvePoints > 0){
                p3 = getStreetSegmentCurvePoint(st_Two.numCurvePoints - 1, dst_street_segment_id);
            }
            else{
                p3 = getIntersectionPosition(st_Two.from);
            }
    }
    
    
    else if (st_One.to == st_Two.from){
        p1 = getIntersectionPosition(st_One.to);
            if (st_One.numCurvePoints > 0){
                p2 = getStreetSegmentCurvePoint(st_One.numCurvePoints - 1, src_street_segment_id);
            }
            else{
                p2 = getIntersectionPosition(st_One.from);
            }
            if (st_Two.numCurvePoints > 0){
                p3 = getStreetSegmentCurvePoint(0, dst_street_segment_id);
            }
            else{
                p3 = getIntersectionPosition(st_Two.to);
            }
    }
    
    
    else if (st_One.from == st_Two.to){
        p1 = getIntersectionPosition(st_One.from);
            if (st_One.numCurvePoints > 0){
                p2 = getStreetSegmentCurvePoint(0, src_street_segment_id);
            }
            else{
                p2 = getIntersectionPosition(st_One.to);
            }
            if (st_Two.numCurvePoints > 0){
                p3 = getStreetSegmentCurvePoint(st_Two.numCurvePoints - 1, dst_street_segment_id);
            }
            else{
                p3 = getIntersectionPosition(st_Two.from);;
            }
    }
    
    
    else if (st_One.from == st_Two.from){
        p1 = getIntersectionPosition(st_One.from);
        if (st_One.numCurvePoints > 0){
            p2 = getStreetSegmentCurvePoint(0, src_street_segment_id);
            }
            else{
                p2 = getIntersectionPosition(st_One.to);
            }
            if (st_Two.numCurvePoints > 0){
                p3 = getStreetSegmentCurvePoint(0, dst_street_segment_id);
            }
            else{
                p3 = getIntersectionPosition(st_Two.to);
            }
    }
    
    else{
        return NO_ANGLE;
    }

    // Convert to lengths a, b, and c
    double a = findDistanceBetweenTwoPoints(p1, p2);
    double b = findDistanceBetweenTwoPoints(p1, p3);
    double c = findDistanceBetweenTwoPoints(p2, p3);

    // Use cosine law to find angle
    return 3.141592 - cosLaw(a, b, c);
}

double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id){
    return street_travel_time[street_segment_id];
}

// Returns all street ids corresponding to street names that start with the
// given prefix.
// The function should be case-insensitive to the street prefix.
// The function should ignore spaces.
//  For example, both "bloor " and "BloOrst" are prefixes to 
//  "Bloor Street East".
// If no street names match the given prefix, this routine returns an empty
// (length 0) vector.
// You can choose what to return if the street prefix passed in is an empty
// (length 0) string, but your program must not crash if street_prefix is a
// length 0 string.
// Speed Requirement --> high

std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix){
    std::vector<StreetIdx> streetMatch;
    if (street_prefix.size() <= 0){
        return streetMatch;
    }
    //Turn prefix to lowercase w no spaces
    std::string prefix = toLowerRemoveSpace(street_prefix);
    char c = prefix[0];

    // Starts at first prefix match using lower_bound
    for (auto it = street_names[c].lower_bound(prefix); it != street_names[c].end(); it++){
        //Compare the two strings
        std::size_t found = it -> first.find(prefix);
        if (found != 0){
            break;
        }
        else{
            streetMatch.push_back(it -> second);
        }
    }
    //Return vector
    return streetMatch;
}


// Returns the area of the given closed feature in square meters.
// Assume a non self-intersecting polygon (i.e. no holes).
// Return 0 if this feature is not a closed polygon.
// Speed Requirement --> moderate

double findFeatureArea(FeatureIdx feature_id){
    int total_points = getNumFeaturePoints(feature_id);
    double total_area = 0;

    // Not a closed polygon
    if (!(getFeaturePoint(0, feature_id) == getFeaturePoint(total_points - 1, feature_id))){
        return 0;
    }
    // Else program will continue
    for (int i = 0; i < total_points - 1; i++){
        LatLon p1 = getFeaturePoint(i, feature_id);
        LatLon p2 = getFeaturePoint(i + 1, feature_id);

        double lat_Avg = (p1.latitude() + p2.latitude())/2;
        double x1 = kEarthRadiusInMeters * kDegreeToRadian * p1.longitude() * cos(kDegreeToRadian*lat_Avg); 
        double x2 = kEarthRadiusInMeters * kDegreeToRadian * p2.longitude() * cos(kDegreeToRadian*lat_Avg); 

        double y1 = kEarthRadiusInMeters * kDegreeToRadian * p1.latitude(); 
        double y2 = kEarthRadiusInMeters * kDegreeToRadian * p2.latitude();

        double temp_area = (x1 + x2)*(y2 - y1)/2;
        total_area += temp_area;
    }

    return abs(total_area);
}

// Returns the length of the OSMWay that has the given OSMID, in meters.
// To implement this function you will have to  access the OSMDatabaseAPI.h
// functions.
// Speed Requirement --> high

double findWayLength(OSMID way_id){
    // Find the OSMWay corresponding to the given ID
    double length = 0;
    auto it = OSM_way_ID.find(way_id);
    if (it != OSM_way_ID.end()){
        std::vector<OSMID> way_members = getWayMembers(it -> second);
        auto it2 = OSM_way_length.find(way_id);
        if (it2 != OSM_way_length.end()){
            length = it2 -> second;
        }
    }
    return length;
}

// Return the value associated with this key on the specified OSMNode.
// If this OSMNode does not exist in the current map, or the specified key is
// not set on the specified OSMNode, return an empty string.
// Speed Requirement --> high

std::string getOSMNodeTagValue(OSMID osm_id, std::string key){
    // Find the OSMNode corresponding to the given ID
    std::string valueFound = "";

    auto it = OSM_node_tags.find(osm_id);
    if (it != OSM_node_tags.end()){
        auto tagmap = it -> second;
        auto tagit = tagmap.find(key);
        if (tagit != tagmap.end()){
            valueFound = tagit -> second;
        }
    }
    return valueFound;
}

// ------------- Nusaiba's Helper Functions -------------

// Get OSMWay tag value
std::string getOSMWayTagValue(OSMID osm_id, std::string key){
    // Find the OSMNode corresponding to the given ID
    std::string valueFound = "";

    auto it = OSM_way_tags.find(osm_id);
    if (it != OSM_way_tags.end()){
        auto tagmap = it -> second;
        auto tagit = tagmap.find(key);
        if (tagit != tagmap.end()){
            valueFound = tagit -> second;
        }
    }
    return valueFound;
}

// Get the node by ID rather than index
const OSMNode* getNodeByID(OSMID osm_id){
    const OSMNode *nodeSearch = NULL;
    for (int i = 0; i < getNumberOfNodes(); i++){
        nodeSearch = getNodeByIndex(i);
        if (nodeSearch != NULL){
            OSMID found_id = nodeSearch -> id();
            if (found_id == osm_id){
                return nodeSearch;
            }
        }
    }
    return nodeSearch;
}

//Remove all spaces from string and convert to lower case
std::string toLowerRemoveSpace(std::string input_string){
    std::string newString;
    int i = 0;
    while (input_string[i]){
        char myChar = input_string[i];
        if (myChar != ' '){
            myChar = tolower(myChar);
            newString += myChar;
        }
        i += 1;
    }
    return newString;
}

//Find an angle in radians given sides a, b, and c using Cosines law
double cosLaw(double a, double b, double c){
    double a2 = a*a;
    double b2 = b*b;
    double c2 = c*c;
    double num = a2 + b2 - c2;
    double denom = 2*a*b;
    double expression = num/denom;
    // Making sure acos doesn't recieve any invalid values due to rounding
    if (expression > 1){
        expression = 1;
    }
    if (expression < -1){
        expression = -1;
    }

    double angle = acos(expression);

    return angle;
}

// Samiyah's Helper Functions

//This function replaces part of a string to another set of chars, via standard lib func, replace, and changes the source string via alias
bool strReplace(std::string& source, const std::string& from_str, const std::string& to_str) {

    size_t startPos = source.find(from_str);

    source.erase(startPos, from_str.length());
    source += to_str;
    return true;
}

