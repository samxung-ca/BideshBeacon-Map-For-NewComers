#include "m3.h"
#include "ezgl/application.hpp"
#include "ezgl/graphics.hpp"
#include "StreetsDatabaseAPI.h"
#include "m1.h"
#include "m2.h"
#include <chrono>
#include <sstream>
#include "globals.h"
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <utility>
#include <unordered_map>
#include "OSMDatabaseAPI.h"
#include "math.h"

#define NO_EDGE -1 // Value for illegal id

// global vector declarations
std::vector<StreetSegmentIdx> emptyVector(0); // empty vector of size == 0 for use in findpath func
std::vector<double> emptyVectorDouble(0); 
std::list<int> emptyList(0);
std::vector<double> travel_times;
std::vector<StreetSegmentIdx> connecting_streets;

//////// function declarations /////////////////////
std::vector<StreetSegmentIdx> bfsTraceBack(int);
std::vector<StreetSegmentIdx> makeVecFromList(std::list<StreetSegmentIdx>&);
std::vector<StreetSegmentIdx> AStarfindPath (int, int, double);
bool findPath (int,int);

struct Node{
    // Outgoing edges
    std::vector<int> outgoingedges;
    // Street segment id of edge used to reach this node
    int reachingEdge; 
    // Shortest time found to this node so far
    double bestTime; 
};

// Vector of nodes
std::vector<Node> nodes;
// Vector to hold shortest path so far
std::vector<StreetSegmentIdx> shortest_path;

// Struct to declare the ints and doubles used later
struct WaveElem{
    // Node to look at next
    int node;
    // Street segment id of edge used to reach node
    int edgeID; 
    // Total travel time to reach node
    double travelTime; 

    // WaveElem constructor
    WaveElem (int n, int id, float time){
        node = n;
        edgeID = id;
        travelTime = time;
    }
};
   
// Struct to sort queue by traveltime - smallest first
struct Greater{
    bool operator() (WaveElem wav1, WaveElem wav2){
        return wav1.travelTime > wav2.travelTime; 
    }
};

 
   
// bool function to see if there is a path or not
bool findPath (int srcID, int destID){
    // Queue to hold nodes to explore next, ordered by those with 
    // Smallest besttimes first to find shortest path quicker
    std::priority_queue<WaveElem, std::vector<WaveElem>, Greater> wavefront; // nodes to explore next
  
    // begin at start
    // assign to wavefront
    wavefront.push(WaveElem(srcID, NO_EDGE, 0));

    // do the following while there are nodes found
    while(!wavefront.empty()){ 
        // int placeholder = 0
        
        //get next node
        WaveElem wave = wavefront.top();
    
        // set currID to first node
        int currID= wave.node;
    
        // pop node from wavefront
        wavefront.pop();

        // Check if the node we're on is smaller than the smallest one we had previously
        if ((wave.travelTime < nodes[currID].bestTime) || (nodes[currID].bestTime==-1)){
            // Get nodes segment id
            nodes[currID].reachingEdge = wave.edgeID;
            // Get nodes time
            nodes[currID].bestTime = wave.travelTime;
     
            // the start and end nodes make a legal path, return that
            if(currID == destID){
                return true;                         
            }
  
            // Get the number of outedges for the node we're on, 
            // to be used as max value in our for loop to go through the outedges
            int outedges_size = nodes[currID].outgoingedges.size();
    
            // Go through outedges of the current node we're on
            for (int i = 0; i < outedges_size; i++){
                // get current nodes street segment id
                int street_segment_id = nodes[currID].outgoingedges[i];
              
                // for each outedge street seg from curr intersection
                // get street segment info of node
                StreetSegmentInfo info= getStreetSegmentInfo(street_segment_id);
           
                //checking for if node starts from a to and it's NOT a one way 
                if (currID == info.to && !info.oneWay){
                    // set the start as from a from
                    int toNodeID = info.from;
                    // get the travel time between the to and from
                    auto time = nodes[currID].bestTime + findStreetSegmentTravelTime(street_segment_id);
                    
                    // update the best time
                    wavefront.push(WaveElem(toNodeID, street_segment_id, time));
                    
                    // nodes[info.from].reachingEdge=outedges[i];
                }
                // otherwise, the node starts from a from
                else if (currID == info.from){
                    // set the start as from a to
                    int fromNodeID = info.to;

                    //get the travel time between the from and to
                    auto time= nodes[currID].bestTime+ findStreetSegmentTravelTime(street_segment_id);
                  
                    wavefront.push(WaveElem(fromNodeID, street_segment_id, time));                  
                }
            }
        }
    }
    // end whole wavefront not empty 
    return false; // no path exists
}


// Returns the time required to travel along the path specified, in seconds.
// The path is given as a vector of street segment ids, and this function can
// assume the vector either forms a legal path or has size == 0. The travel
// time is the sum of the length/speed-limit of each street segment, plus the
// given turn_penalty (in seconds) per turn implied by the path. If there is
// no turn, then there is no penalty. Note that whenever the street id changes
// (e.g. going from Bloor Street West to Bloor Street East) we have a turn.
double computePathTravelTime(const double turn_penalty, const std::vector<StreetSegmentIdx>& path){
    if (path.size() == 0){
        return 0.0;
    }
    else{
        double seg_div_speedlim = 0.00;
        StreetIdx streetprev = getStreetSegmentInfo(path[0]).streetID;
        for (int i = 0; i < path.size(); i++){
            StreetSegmentInfo info = getStreetSegmentInfo(path[i]);
            
            seg_div_speedlim += findStreetSegmentTravelTime(path[i]);
        
            if (streetprev != info.streetID){
                seg_div_speedlim += turn_penalty;
            }
            streetprev = info.streetID;
        }
        return seg_div_speedlim; 
    }
}

// Vector to return the list used in bfsTraceBack() as a vector
std::vector<StreetSegmentIdx> makeVecFromList(std::list<StreetSegmentIdx>& path){
    return std::vector<StreetSegmentIdx>(path.begin(),path.end());
}

// Function to trace back through the nodes
std::vector<StreetSegmentIdx> bfsTraceBack (int destID) {
    std::list <StreetSegmentIdx> path;
    
    int currNodeID = destID;
    StreetSegmentIdx prevEdge = nodes[currNodeID].reachingEdge;
    
    while (prevEdge!= NO_EDGE){
        path.push_front(prevEdge);
        // currNodeID is the intersection on the other side of reaching edge street seg not yet used
        if (getStreetSegmentInfo(prevEdge).to==currNodeID){
            currNodeID= getStreetSegmentInfo(prevEdge).from;
        }
        else{
            currNodeID=getStreetSegmentInfo(prevEdge).to;
        }
        // currNodeID = 
        prevEdge = nodes[currNodeID].reachingEdge;
    }
    
    return makeVecFromList(path);   
}

// Returns a path (route) between the start intersection (intersect_id.first)
// and the destination intersection (intersect_id.second), if one exists.
// This routine should return the shortest path
// between the given intersections, where the time penalty to turn right or
// left is given by turn_penalty (in seconds). If no path exists, this routine
// returns an empty (size == 0) vector. If more than one path exists, the path 
// with the shortest travel time is returned. The path is returned as a vector 
// of street segment ids; traversing these street segments, in the returned 
// order, would take one from the start to the destination intersection.
std::vector<StreetSegmentIdx> findPathBetweenIntersections(const double turn_penalty, const std::pair<IntersectionIdx, IntersectionIdx> intersect_ids){
    int num_intersections = getNumIntersections();
    
    nodes.clear();
    nodes.resize(num_intersections);
    
    for (int i = 0; i < num_intersections; i++){
        // int street_segments_of_intersection = findStreetSegmentsOfIntersection(i);
        nodes[i].outgoingedges = findStreetSegmentsOfIntersection(i);
        nodes[i].bestTime = -1;
        // nodes[i].already_visited = false;
    }
    
    int start = intersect_ids.first;
    int end = intersect_ids.second;
  
    if(findPath(start, end) == true){
      shortest_path = bfsTraceBack(end);
    }
    
    computePathTravelTime(turn_penalty, shortest_path);
    
    return shortest_path;
}
