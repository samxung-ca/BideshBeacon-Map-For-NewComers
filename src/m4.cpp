/**
 * @brief A simple pathfinding function with no turn penalty
 * This function will not compile or run until after Milestone 3 
 * is over.
 *
 * If you do not have a working findPathBetweenIntersections from
 * milestone 3, this file includes a Djikstra algorithm with no turn 
 * penalties that you can use for milestone 4.
 *
 * The function prototype is:
 * std::vector<StreetSegmentIdx> findSimplePath(
 *           const std::pair<IntersectionIdx, IntersectionIdx> intersect_ids);
 *
 * The function is calling 2 of your own milestone 1 functions:
 *	- findStreetSegmentsOfIntersection
 *	- findStreetSegmentTravelTime
 * so those functions must be implemented and correct.
 *
 * @note If you have a working solution from milestone 3, it is better to use 
 *       it instead of findSimplePath, as it will give smaller travel times 
 *       due to handling turn penalties, and it may also have lower cpu time.
 */
#include "FindSimplePath.h"
#include "m4.h"
#include "m3.h"
#include "globals.h"
#include <algorithm>
#include <omp.h>
#include <cstdlib>

#define NO_DROP -1
#define TIME_LIMIT 50000 // Autotester time limit in milliseconds

struct locationInfo{
    // ID of the delivery location
    IntersectionIdx loc;
    // The corresponding dropoff location. If the loc
    // is a dropOff itself, or a depot
    // the value is -1 aka NO_DROP
    IntersectionIdx dropOff;
    // Map storing the distances between the 
    // location and all other delivery locations
    std::multimap<double, IntersectionIdx> dist;
    // Whether the current delivery has been processed
    bool processed;

    // Constructor
    locationInfo (IntersectionIdx i, IntersectionIdx drop, std::multimap<double, IntersectionIdx> distances, bool process){
        loc = i;
        dropOff = drop;
        dist = distances;
        processed = process;
    }

    bool operator==(const locationInfo& rhs){
        return (loc == rhs.loc) && (dropOff == rhs.dropOff) && (processed == rhs.processed);
    }
};

//vector to hold intersection ids
std::vector<IntersectionIdx> intersection_ids;
//vector to hold path, from struct in m4.h
std::vector<CourierSubPath> path;
//vector to hold paths 
std::vector<std::vector<StreetSegmentIdx>> paths;
//precomputed distances matrices
//std::unordered_map<IntersectionIdx,std::unordered_map<IntersectionIdx, std::vector<StreetSegmentIdx>>> paths;
//vector to hold nodes of current path
std::vector<IntersectionIdx> path_nodes;
//vector to hold the distances between the path nodes
std::vector<IntersectionIdx> dist_btw_path_nodes;
//empty vector global
std::vector<CourierSubPath> emptyVec(0); 
std::vector<StreetSegmentIdx> emptySeg(0); 
// For holding precomupted geometric distances between all delivery locations
// std::unordered_map<IntersectionIdx, std::map<double, locInfo>> delivery_dist;
// For holding precomupted geometric distances between each depot and all delivery locations
// std::unordered_map<IntersectionIdx, std::map<double, locInfo>> depot_dist;
// std::vector<locationInfo> delivery_list;
std::unordered_map<IntersectionIdx, std::multimap<double, IntersectionIdx>> delivery_list;
std::unordered_map<IntersectionIdx, IntersectionIdx> depot_list;

// Helper functions
std::vector<CourierSubPath> IdxtoPath(const float turn_penalty, std::vector<IntersectionIdx>&path);
//vector func for simulated annealing
std::vector<int>simulated_annealing(std::vector<int>&, std::vector<double>&);
//function to calulate the cost of the path
double costOfPath(std::vector<int>&, std::vector<double>&);
//struct nodes
struct Node{
    // Outgoing edges
    std::vector<int> outgoingedges;
    // Street segment id of edge used to reach this node
    int reachingEdge; 
    // Shortest time found to this node so far
    double bestTime; 
};

std::vector<IntersectionIdx> startPath(const std::vector<DeliveryInf>& deliveries, const IntersectionIdx start_depot, const IntersectionIdx start_pickup);

bool isLegal(std::unordered_multimap<IntersectionIdx, IntersectionIdx> legal_deliveries, IntersectionIdx loc);
void preloadDistances(const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots);
// locationInfo findNext(IntersectionIdx next);
void multiDestDijkstra (int turn_penalty, const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots);
std::vector<StreetSegmentIdx> find_paths(int turn_penalty, int src, std::vector<IntersectionIdx> dests);
double pathCost(const float turn_penalty, std::vector<CourierSubPath>&path);
void perturb(std::vector<IntersectionIdx>& curr_path);
void legalPath(const std::vector<DeliveryInf>& deliveries, std::vector<IntersectionIdx>& curr_path);

// locationInfo findNext(IntersectionIdx next){
//     for (int i = 0; i < delivery_list.size(); i++){
//         if (delivery_list[i].loc == next){
//             return delivery_list[i];
//         }
//     }
// }

// Preloads the geometric distances between all delivery points and depots
void preloadDistances(const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots){
    // Load depot_list with closest pickup location for each depot
    double min_dist = 100000000000000000000;
    IntersectionIdx min_loc = 0;
    for (int i = 0; i < depots.size(); i++){
        LatLon pd = getIntersectionPosition(depots[i]);
        for (int j = 0; j < deliveries.size(); j++){
            LatLon p1 = getIntersectionPosition(deliveries[j].pickUp);
            double dist = findDistanceBetweenTwoPoints(pd, p1);

            if (dist < min_dist){
                min_dist = dist;
                min_loc = deliveries[j].pickUp;
            }
        }
        depot_list.insert(std::make_pair(depots[i], min_loc));
    }

    // Load delivery_list with geometric distances
    //delivery_list.resize(getNumIntersections());
    for (int k = 0; k < deliveries.size(); k++){
        IntersectionIdx loc1 = deliveries[k].pickUp;
        LatLon p1 = getIntersectionPosition(loc1);
        IntersectionIdx loc2 = deliveries[k].dropOff;
        LatLon p2 = getIntersectionPosition(loc2);
        std::multimap<double, IntersectionIdx> map1;
        std::multimap<double, IntersectionIdx> map2;
        
        double dist = findDistanceBetweenTwoPoints(p1, p2);
        map1.insert(std::make_pair(dist, loc2));
        map2.insert(std::make_pair(dist, loc1));

        for (int n = 0; n < deliveries.size(); n++){
            if (k != n){
                IntersectionIdx loc3 = deliveries[n].pickUp;
                IntersectionIdx loc4 = deliveries[n].dropOff;
                LatLon p3 = getIntersectionPosition(loc3);
                LatLon p4 = getIntersectionPosition(loc4);

                double dist1 = findDistanceBetweenTwoPoints(p1, p3);
                double dist2 = findDistanceBetweenTwoPoints(p1, p4);
                map1.insert(std::make_pair(dist1, loc3));
                map1.insert(std::make_pair(dist2, loc4));

                double dist3 = findDistanceBetweenTwoPoints(p2, p3);
                double dist4 = findDistanceBetweenTwoPoints(p2, p4);
                map2.insert(std::make_pair(dist3, loc3));
                map2.insert(std::make_pair(dist4, loc4));
            }
        }

        delivery_list.insert(std::make_pair(loc1, map1));
        delivery_list.insert(std::make_pair(loc2, map2));
        //delivery_list.push_back(locationInfo(loc1, loc2, map1, false));
        //delivery_list.push_back(locationInfo(loc2, NO_DROP, map2, false));
    }
}

/*
//Specifies a delivery order (input to your algorithm).
//To satisfy the order the item-to-be-delivered must have been picked-up 
//from the pickUp intersection before visiting the dropOff intersection.
struct DeliveryInf {

    // Constructor
    DeliveryInf(IntersectionIdx pick_up, IntersectionIdx drop_off)
        : pickUp(pick_up), dropOff(drop_off) {}

    //The intersection id where the item-to-be-delivered is picked-up.
    IntersectionIdx pickUp;

    //The intersection id where the item-to-be-delivered is dropped-off.
    IntersectionIdx dropOff;
};


// Specifies one subpath of the courier truck route
struct CourierSubPath {
    // The intersection ids defining the start and end of the subpath.
    // The first intersection id is where a start depot, pick-up intersection,
    // or drop-off intersection is located.
    // The second intersection id is where this subpath ends. This must be the
    // first intersection of the next subpath or the intersection of an end depot.
    std::pair<IntersectionIdx, IntersectionIdx> intersections;

    // Street segment ids of the path between the start and end intersection
    // They form a connected path (see m3.h)
    std::vector<StreetSegmentIdx> subpath;
};
*/

// This routine takes in a vector of D deliveries (pickUp, dropOff
// intersection pairs), another vector of N intersections that
// are legal start and end points for the path (depots), and a turn 
// penalty in seconds (see m3.h for details on turn penalties).
//
// The first vector 'deliveries' gives the delivery information.  Each delivery
// in this vector has pickUp and dropOff intersection ids.
// A delivery can only be dropped-off after the associated item has been picked-up. 
// 
// The second vector 'depots' gives the intersection ids of courier company
// depots containing trucks; you start at any one of these depots but you must
// end at the same depot you started at.
//
// This routine returns a vector of CourierSubPath objects that form a delivery route.
// The CourierSubPath is as defined above. The first street segment id in the
// first subpath is connected to a depot intersection, and the last street
// segment id of the last subpath also connects to a depot intersection.
// A package will not be dropped off if you haven't picked it up yet.
//
// The start_intersection of each subpath in the returned vector should be 
// at least one of the following (a pick-up and/or drop-off can only happen at 
// the start_intersection of a CourierSubPath object):
//      1- A start depot.
//      2- A pick-up location
//      3- A drop-off location. 
//
// You can assume that D is always at least one and N is always at least one
// (i.e. both input vectors are non-empty).
//
// It is legal for the same intersection to appear multiple times in the pickUp
// or dropOff list (e.g. you might have two deliveries with a pickUp
// intersection id of #50). The same intersection can also appear as both a
// pickUp location and a dropOff location.
//        
// If you have two pickUps to make at an intersection, traversing the
// intersection once is sufficient to pick up both packages. Additionally, 
// one traversal of an intersection is sufficient to drop off all the 
// (already picked up) packages that need to be dropped off at that intersection.
//
// Depots will never appear as pickUp or dropOff locations for deliveries.
//  
// If no valid route to make *all* the deliveries exists, this routine must
// return an empty (size == 0) vector.
std::vector<CourierSubPath> travelingCourier(const float turn_penalty,const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots){
    preloadDistances(deliveries, depots);
    
    std::vector<CourierSubPath> c_path;
    int deliveries_size = deliveries.size();
    int depots_size = depots.size();

    if (deliveries_size == 0){
        return emptyVec;
    }
    if (depots_size == 0){
        return emptyVec;
    }

    std::vector<IntersectionIdx> i_path;
    std::vector<IntersectionIdx> i_path_temp;
    std::vector<CourierSubPath> c_path_temp;

    auto start = std::chrono::high_resolution_clock::now();
    
    i_path = startPath(deliveries, depots[0], deliveries[0].pickUp);
    c_path = IdxtoPath(turn_penalty, i_path);
    ////Multi-Dest Dijkstra dec into c_path, without perturb; crash n mem issue
    /*
    for (int i = 0; i < c_path.size(); ++i) {
        // Retrieve the precomputed path for each subpath
        IntersectionIdx start_intersection = c_path[i].intersections.first;
        IntersectionIdx end_intersection = c_path[i].intersections.second;

        // Find the precomputed path between start and end intersections
        std::vector<StreetSegmentIdx>& precomputed_path = paths[start_intersection];//[end_intersection];

        // Update the subpath with the precomputed path
        c_path[i].subpath = precomputed_path;
    }
    */
    double curr_cost = pathCost(turn_penalty, c_path);
    double new_cost;

    auto now = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    int d = 1;
    int n = 1;

    // std::map<>
    //while (delta.count() < 0.4*TIME_LIMIT){
        //#pragma omp parallel for
        //for (int i = 0; i < 2000; i++){
        while (delta.count() < 0.5*TIME_LIMIT){
            // n = rand()%depots_size;
            d = rand()%deliveries_size;
            i_path_temp = startPath(deliveries, depots[0], deliveries[d].pickUp);
            // depot_list.find(depots[n])->second
            c_path_temp = IdxtoPath(turn_penalty, i_path_temp);
            new_cost = pathCost(turn_penalty, c_path_temp);

            if (new_cost < curr_cost){
                c_path = c_path_temp;
                i_path = i_path_temp;
                curr_cost = new_cost;
            }

            now = std::chrono::high_resolution_clock::now();
            delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            //std::cout << "MULTISTART | Current time: " << delta.count() << ", Current cost: " << curr_cost << std::endl;
            //n++;
        }
        //d++;
    //}

    if (curr_cost > 80000){
        i_path_temp = i_path;
        std::vector<IntersectionIdx> i_path_iter;
        std::vector<CourierSubPath> c_path_iter;
        double iter_cost;
        int t = 123456;

        while (delta.count() < 0.8*TIME_LIMIT){
            i_path_iter = i_path_temp;
            perturb(i_path_iter);
            legalPath(deliveries, i_path_iter);

            
            c_path_iter = IdxtoPath(turn_penalty, i_path_iter);
            iter_cost = pathCost(turn_penalty, c_path_iter);
            
            if (iter_cost < curr_cost){
                curr_cost = iter_cost;
                c_path = c_path_iter;
                i_path = i_path_iter;

                i_path_temp = i_path_iter;
            }
            else{
                if (rand()%2 < exp((curr_cost - iter_cost)/t)){
                    i_path_temp = i_path_iter;
                }
            }

            now = std::chrono::high_resolution_clock::now();
            delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            //std::cout << "ANNEALING | Current time: " << delta.count() << ", Current cost: " << curr_cost << ", Current iter_cost: " << iter_cost<< std::endl;
        }
    }

    return c_path;
}

void perturb(std::vector<IntersectionIdx>& curr_path){
    int size = curr_path.size();
    int i1 = rand()%size;
    int i2 = rand()%size;

    while (i1 == 0 || i1 == size - 1){
        i1 = rand()%size;
    }

    while (i2 == i1 || i2 == 0 || i2 == size - 1){
        i2 = rand()%size;
    }

    if (i1 > i2){
        int temp = i1;
        i1 = i2;
        i2 = temp;
    }

    std::vector<IntersectionIdx> new_path;
    new_path.push_back(curr_path[0]);
    for (int i = 1; i < size - 1; i++){
        if (i < i1){
            new_path.push_back(curr_path[i]);
        }
        else if (i == i1){
            new_path.push_back(curr_path[i2]);
        }
        else if (i > i1 && i <= i2){
            new_path.push_back(curr_path[i - 1]);
        }
        else{
            new_path.push_back(curr_path[i]);
        }
    }
    new_path.push_back(curr_path[0]);

    curr_path = new_path;
}

void legalPath(const std::vector<DeliveryInf>& deliveries, std::vector<IntersectionIdx>& curr_path){
    bool legal = true;
    for (int i = 0; i < deliveries.size(); i++){
        auto it1 = std::find(curr_path.begin(), curr_path.end(), deliveries[i].pickUp);
        auto it2 = std::find(curr_path.begin(), curr_path.end(), deliveries[i].dropOff);
        if (it1 > it2){
            std::iter_swap(it1, it2);
        }
    }
}

std::vector<IntersectionIdx> startPath(const std::vector<DeliveryInf>& deliveries, const IntersectionIdx start_depot, const IntersectionIdx start_pickup){
    // Initial Setup
    std::vector<IntersectionIdx> i_path;
    std::unordered_multimap<IntersectionIdx, IntersectionIdx> to_deliver;
    
    // std::cout << deliveries.size() << std::endl;
    for (int i = 0; i < deliveries.size(); i++){
        to_deliver.insert(std::make_pair(deliveries[i].pickUp, deliveries[i].dropOff));
    }
    
    i_path.push_back(start_depot);

    IntersectionIdx curr_loc = start_pickup;
    i_path.push_back(curr_loc);
    
    auto range = to_deliver.equal_range(curr_loc);
    for (auto it = range.first; it != range.second; it++){
        IntersectionIdx drop = it->second;
        if (drop != NO_DROP){
            to_deliver.insert(std::make_pair(drop, NO_DROP));
        }
    }
    to_deliver.erase(curr_loc);

    //locationInfo info = findNext(curr_loc);

    // Finding path between all deliveries and then back to depot
    
    while (to_deliver.size() > 0){
        std::multimap<double, IntersectionIdx> dist_map = delivery_list[curr_loc];
        bool first = false;
        for (auto deliver_next = dist_map.begin(); deliver_next != dist_map.end(); deliver_next++){
            curr_loc = deliver_next->second;
            if (isLegal(to_deliver, curr_loc)){
                if (first == true || rand()%100 < 97){
                    // info = findNext(curr_loc);
                    break;
                }
                else{
                    first = true;
                }
            }
        }

        i_path.push_back(curr_loc);
        //to_deliver.insert(std::make_pair(info.dropOff, NO_DROP));

        auto range2 = to_deliver.equal_range(curr_loc);
        for (auto it2 = range2.first; it2 != range2.second; it2++){
            IntersectionIdx drop = it2->second;
            if (drop != NO_DROP){
                to_deliver.insert(std::make_pair(drop, NO_DROP));
            }
        }
        to_deliver.erase(curr_loc);
    }

    i_path.push_back(start_depot);

    return i_path;
}

bool isLegal(std::unordered_multimap<IntersectionIdx, IntersectionIdx> legal_deliveries, IntersectionIdx loc){
    bool legal = false;
    for (auto it = legal_deliveries.begin(); it != legal_deliveries.end(); it++){
        if (it->first == loc){
            legal = true;
        }
    }

    return legal;
}

double pathCost(const float turn_penalty, std::vector<CourierSubPath>&path){
    double travel_time = 0;
    for (int i = 0; i < path.size(); i++){
        travel_time += computePathTravelTime(turn_penalty, path[i].subpath);
    }
    return travel_time;
}

std::vector<CourierSubPath> IdxtoPath(const float turn_penalty, std::vector<IntersectionIdx>&path){
    std::vector<CourierSubPath> c_path;
    c_path.resize(path.size() - 1);

    for (int i = 0; i < c_path.size(); i++){
        auto pair = std::make_pair(path[i], path[i + 1]);
        c_path[i].intersections = pair;
        c_path[i].subpath = findPathBetweenIntersections(turn_penalty, pair);
    }

    return c_path;
}

// Simulated annealing
std::vector<int> simulated_annealing(std::vector<int>&path, std::vector<double>&dist_btw_path_nodes){
    
    std::vector<int> current_path = path; // current path 
    
    //the following line is from "double s = InitialSolution;" in lecture 
    std::vector<int> simulated_annealing_path = path; // best path
    
    //the following line is from "double C = Cost(S);" in lecture
    double simulated_annealing_cost = costOfPath(simulated_annealing_path,dist_btw_path_nodes);
 
    //double T = high_temperature;
    double minTemp = 1; // min temp to start at
    double maxTemp = 123456; // high temp
    double coolingTemp = (1/100); // small temp
    double coolingTemp_rate = 1 - coolingTemp; // ratio of cooling temp
    double e = 2.71828182845904523536; // value of e, used in the if statement later in this function
    
while (minTemp < maxTemp){
//for(many perturbutions){
// double S_new = perturb(S);
    
//the following line is from "double C_new = cost(S);" given in lecture
  double simulated_annealing_cost_new = costOfPath(simulated_annealing_path,dist_btw_path_nodes);
  
//the following line is from "change_in_C = C_new - C;" in lecture 
  double change_in_simulated_annealing_cost = simulated_annealing_cost_new - simulated_annealing_cost;
  
  double power = ((-1)*(change_in_simulated_annealing_cost))/maxTemp; // value to be used as -change_in_C/T in the following if statement
  
  double e_power = pow(e, power);
  
  //int path_index1, path_index2; //possibilities for path indexes randomly chosen
    
//the following line is from "if(C_new < C || rand(0,1) < e^(-change_in_C/T)){" in lecture
  if ( (simulated_annealing_cost_new < simulated_annealing_cost) || (rand()) < (e_power)){
// the following line is from "S = S_new;" in lec
    current_path = simulated_annealing_path;  
// the following line is from "C = C_new;'" in lecture
    // setting the cost as the new cost which is less
    simulated_annealing_cost = simulated_annealing_cost_new;

    // if the new cost we found is actually more than our original one, do the opposite
    if (simulated_annealing_cost < simulated_annealing_cost_new){
         simulated_annealing_path = current_path;  

         simulated_annealing_cost_new = simulated_annealing_cost;
    }
    // update the temperature tp next lowest level 
    maxTemp = maxTemp * coolingTemp_rate;
}
//T = reduceTemp(T_;
    return simulated_annealing_path;
}
}

// Calulating cost of path
double costOfPath(std::vector<int>& path, std::vector<double>&dist_btw_path_nodes){
    double cost_of_path = 0.0;
    double path_size = dist_btw_path_nodes.size();
    for (int i = 0; i < path_size; i++){
        cost_of_path = cost_of_path + dist_btw_path_nodes[i];
    }
    return cost_of_path;
}


std::vector<IntersectionIdx> key_intersections;

////preload time_matrix using multidest dijkstra algorithm in the find_paths algorithm
void multiDestDijkstra (int turn_penalty, const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots){
    key_intersections.resize(deliveries.size()+deliveries.size()+depots.size());
    
    for (int i=0; i<deliveries.size(); i++){
        key_intersections.push_back(deliveries[i].pickUp);
        key_intersections.push_back(deliveries[i].dropOff);
    }

    for (int i=0; i<depots.size(); i++){
        key_intersections.push_back(depots[i]);
    }
    
    // OpenMP for parallel computation
    #pragma omp parallel for 
    for (auto src: key_intersections){   // shorthand for for (int i=0; i<key_intersects.size(); ++i), int src= key_intersects[i];
        // Find paths from the current source intersection to all dests
        paths[src] = find_paths(turn_penalty, src, key_intersections);
    }
}

//returns a path to one of the key_intersection to locations
std::vector<StreetSegmentIdx> find_paths(int turn_penalty, int src, std::vector<IntersectionIdx> dests){
    //std::priority_queue waveFront; //keep privatized to keep thread safe
   
    std::vector <Node> nodes;//keep privatized to keep thread safe
    std::vector<std::vector<StreetSegmentIdx>> pathsFromSrc;
    std::vector<int> time;
    
    //resize vectors to hold paths from src intersection
    pathsFromSrc.resize(key_intersections.size());
    
    //for each src, compute shortest path for all other possible destinations (key_intersections)
    for (int i=0; i<key_intersections.size(); i++){
        if (key_intersections[i]!=src){
        std::pair<IntersectionIdx,IntersectionIdx> pairsend (std::make_pair(src, key_intersections[i]));
        std:: vector<StreetSegmentIdx> pathFsrc= findPathBetweenIntersections (turn_penalty, pairsend);
        pathsFromSrc.push_back(pathFsrc);
        time.push_back(computePathTravelTime(turn_penalty, pathFsrc));
        }
    }
    
    int timeprev=time[0];
    int previndex=0;
    for (int i=0; i<time.size();i++){
        if (time[i]<timeprev){
            timeprev=time[i];
            previndex=i;
        }
    }
    
    return pathsFromSrc[previndex];
    
}

//everything
/*
void time(const std::vector<DeliveryInf>& deliveries, const std::vector<IntersectionIdx>& depots){
    
    key_intersections.resize(deliveries.size()+depots.size());
    
    for (int i=0; i<deliveries.size; i++){
        key_intersections[i]= deliveries[i];
    }
    for (int i=0; i<depots.size; i++){
        key_intersections[i]= depots.[i];
    }
}
 * */