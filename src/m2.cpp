
#include "globals.h"

// Function declarations used in drawMap
void draw_main_canvas(ezgl::renderer *g);
void act_on_mouse_click (ezgl::application* app, GdkEventButton* event, double x, double y);
void initial_setup(ezgl::application* application, bool);

//Map Drawing Helper Functions
void drawFeatures(ezgl::renderer*, double scale_factor);
void drawStreets(ezgl::renderer*, double scale_factor);
void drawIntersections(ezgl::renderer*);
void drawPOIs(ezgl::renderer*);
void drawStreetLines(ezgl::renderer*, int, StreetSegmentInfo);
void setStreetStyle(ezgl::renderer*, std::string, std::string, double, bool);
void drawStreetNames(ezgl::renderer * g, double scale_factor);
double getSlope(StreetSegmentInfo);

// UI Helper Functions
void loadCss(const gchar*);
void menuPopup(const gchar*, ezgl::application*);
void helpDoc(GtkButton*, ezgl::application*);
void nightMode(GtkButton*, ezgl::application*);
void findIntersections(GtkButton*, ezgl::application*);
void chooseMap(GtkButton*, ezgl::application*);
void changeMap(GtkComboBox*, ezgl::application*);
void changeLang(GtkButton*, ezgl::application*);
void navMenu(GtkButton*, ezgl::application*);
void layerToggle(GtkButton*, ezgl::application*);
void twoIntersections(GtkButton*, ezgl::application*);
void refreshMap(ezgl::application*);
void enterSearch(GtkEntry*, ezgl::application*);
void langChoice(GtkComboBox*, ezgl::application*);
void poiToggle(GtkWidget*, GdkEventButton, gpointer, ezgl::application*);
bool isVisible(ezgl::point2d, ezgl::renderer*);
void submitNav(GtkButton*, ezgl::application*);
void submitNav2(GtkButton*, ezgl::application*);

// M3 UI Helper Functions 
void drawPath(std::vector<StreetSegmentIdx>, ezgl::renderer*, double);
void addDirection(const gchar*, int, int, ezgl::application*);
void firstIntersection(ezgl::application*);
void secondIntersection(ezgl::application*);
void deleteDirections(ezgl::application*);
int findDirection(StreetSegmentIdx src, StreetSegmentIdx dest);

// Global Variables
bool night = false;
bool nav = false;
bool inter_first = false;
bool directions = false;
double max_width;
int POI_prev = 0;
int inter_last = 0;
int num_POI_categories = 31;
int num_directions = 0;
IntersectionIdx inter_one, inter_two;

// UI related data structures
std::vector<gboolean> POI_on;
std::vector<StreetSegmentIdx> nav_segments;

// Constants
#define NO_DATA -1

// Draws the currently loaded map. Must load it first with loadMap()
void drawMap() {
    ezgl::application::settings settings;
    
    settings.main_ui_resource = "libstreetmap/resources/main.ui";
    settings.window_identifier = "MainWindow";
    settings.canvas_identifier = "MainCanvas";
    
    // Create ezgl application with our settings defined
    ezgl::application application (settings);
    
    // Initialize important data structures and variables
    POI_on.resize(num_POI_categories);

    max_width = x_from_lon(max_lon) - x_from_lon(min_lon);
    inter_one = NO_DATA;
    inter_two = NO_DATA;

    // Create a rectangle from left bottom corner to upper right corner
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},{x_from_lon(max_lon), y_from_lat(max_lat)}); 
  
    // Define canvas
    application.add_canvas("MainCanvas", draw_main_canvas, initial_world);

    // Run the application until the user quits
    application.run(initial_setup, act_on_mouse_click, nullptr, nullptr);
}

// Draws/renders the map features onto MainCanvas
void draw_main_canvas(ezgl::renderer *g){

    // Determine current scale for help with drawing 
    // map elements
    ezgl::rectangle world = g -> get_visible_world();
    double curr_width = world.m_second.x - world.m_first.x;
    double scale_factor = curr_width / max_width;

    // Set background colour
    if (night){
        g->set_color(43, 41, 36);
    }
    else{
        g->set_color(252, 249, 247);
    }
    g->fill_rectangle({
            x_from_lon(min_lon),
            y_from_lat(min_lat)
            }, {
            x_from_lon(max_lon),
            y_from_lat(max_lat)
    });

    // Draw map elements
    drawFeatures(g, scale_factor);
    drawStreets(g, scale_factor);
    drawPOIs(g);
    drawIntersections(g);

    // Draw navigation path
    if (nav_segments.size() > 0 && nav == true && inter_first == false) {
        drawPath(nav_segments, g, scale_factor);
    }

    // Draw street names above streets and
    // navigation path
    drawStreetNames(g, scale_factor);

    // Draw navigation path start and end pins
    if (nav == true){
        // If the first pin is being selected, draw it on top
        if (inter_first == false){
            if (inter_two != NO_DATA){
                ezgl::point2d pos_two = intersection_data[inter_two].pos; 
                ezgl::surface* icon_two = g->load_png("libstreetmap/resources/directions/2.png");
                g->draw_surface(icon_two, pos_two, 0.4);
            }
            if (inter_one != NO_DATA){
                ezgl::point2d pos_one = intersection_data[inter_one].pos; 
                ezgl::surface* icon_one = g->load_png("libstreetmap/resources/directions/1.png");
                g->draw_surface(icon_one, pos_one, 0.4);
            }
        }
        // If the second pin is being selected, 
        // draw that on top instead
        else{
            if (inter_one != NO_DATA){
                ezgl::point2d pos_one = intersection_data[inter_one].pos; 
                ezgl::surface* icon_one = g->load_png("libstreetmap/resources/directions/1.png");
                g->draw_surface(icon_one, pos_one, 0.4);
            }
            // if (inter_two != NO_DATA){
            //     ezgl::point2d pos_two = intersection_data[inter_two].pos; 
            //     ezgl::surface* icon_two = g->load_png("libstreetmap/resources/directions/2.png");
            //     g->draw_surface(icon_two, pos_two, 0.4);
            // }
        }
    }
}

// ------------------------------- Map Drawing Helper Functions --------------------------------

// Reloads/refreshes certain data and processes for loading a new map
void refreshMap(ezgl::application* application) {
    // Create initial world rect from new map coordinates
    ezgl::rectangle initial_world({x_from_lon(min_lon), y_from_lat(min_lat)},{x_from_lon(max_lon), y_from_lat(max_lat)});     

    max_width = x_from_lon(max_lon) - x_from_lon(min_lon);
    inter_one = NO_DATA;
    inter_two = NO_DATA;
  
    // redefine canvas
    application->change_canvas_world_coordinates("MainCanvas", initial_world);
}

// A helper function called in draw_main_canvas
// Draws map features such as lakes, parks etc
void drawFeatures(ezgl::renderer *g, double scale_factor){
    for (int i = 0; i < getNumFeatures(); i++){
        std::vector<ezgl::point2d> feature_vert = feature_data[i].second;
        double area = feature_data[i].first;
        bool show = false;

        // Only draw the current feature if these conditions are met
        if (scale_factor < 0.15 || area > 10000 || (scale_factor < 0.25 && area > 200)){
            // Determine whether any part of the feature is in visible range
            for (int j = 0; j < feature_vert.size(); j++){
                if (isVisible(feature_vert[j], g) == true){
                    show = true;
                }
            }

            // If visible, then print
            if (show == true){
                // Set colour based on feature type
                if (getFeatureType(i) == PARK){
                    if (night){
                        g->set_color (0, 80, 0); 
                    }
                    else{
                        g->set_color (142, 232, 132); 
                    }
                }
                else if (getFeatureType(i) == BEACH){
                    if (night){
                        g->set_color (91, 84, 54); 
                    }
                    else {
                        g->set_color (250, 236, 187); 
                    }
                }
                else if (getFeatureType(i) == LAKE){
                    if (night){
                        g->set_color (7, 25, 47);
                    }
                    else{
                        g->set_color (161, 207, 247);
                    }
                }
                else if (getFeatureType(i) == RIVER){
                    if (night){
                        g->set_color (49, 82, 174);
                    } 
                    else{
                        g->set_color (111, 184, 247);
                    }
                }
                else if (getFeatureType(i) == ISLAND){
                    if (night){
                        g->set_color (41, 70, 16);
                    }
                    else{
                        g->set_color (200, 222, 149);
                    }
                }
                else if (getFeatureType(i) == BUILDING){
                    if (night){
                        //g->set_color (73, 71, 66);
                        g->set_color(58, 55, 120);
                    }
                    else{
                        g->set_color (232, 228, 235);
                    }
                }
                else if (getFeatureType(i) == GREENSPACE){
                    if (night){
                        g->set_color (22, 70, 16);
                    }
                    else{
                        g->set_color (181, 222, 149);
                    }
                }
                else if (getFeatureType(i) == GOLFCOURSE){
                    if (night){
                        g->set_color (0, 44, 0); 
                    }
                    else{
                        g->set_color (127, 196, 120); 
                    }
                }
                else if (getFeatureType(i) == STREAM){
                    if (night){
                        g->set_color (2, 79, 114); 
                    }
                    else{
                        g->set_color (161, 231, 247); 
                    }
                }
                else if (getFeatureType(i) == GLACIER){
                    if (night){
                        g->set_color (83, 93, 119); 
                    }
                    else{
                        g->set_color (242, 245, 252); 
                    }
                }
                
                // Draw the feature given its vertices
                if(feature_vert.size() > 1){
                    if ((feature_vert[0].x == feature_vert[getNumFeaturePoints(i)-1].x) && (feature_vert[0].y == feature_vert[getNumFeaturePoints(i)-1].y)){
                        g->fill_poly(feature_vert);
                    }
                }
            }
        }
    }
}

// A helper function called in draw_main_canvas
// Uses other helper functions to draw streets
void drawStreets(ezgl::renderer * g, double scale_factor) {
    // Draw smaller streets first 
    for (StreetSegmentIdx i = 0; i < segment_intersections.size(); i++) {
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        std::string type = getOSMWayTagValue(info.wayOSMID, "highway");
        std::string st_name = getStreetName(info.streetID);

        if (scale_factor < 0.15 && (type == "tertiary" || type == "residential" || type == "unclassified" || st_name == "<unknown>" )) {
            ezgl::point2d med;
            //from + (to - from)/2
            med.x = segment_intersections[i].first.pos.x + ((segment_intersections[i].second.pos.x - segment_intersections[i].first.pos.x) / 2);
            med.y = segment_intersections[i].first.pos.y + ((segment_intersections[i].second.pos.y - segment_intersections[i].first.pos.y) / 2);
            if (isVisible(segment_intersections[i].first.pos, g) || isVisible(segment_intersections[i].second.pos, g) || isVisible(med, g)){
                setStreetStyle(g, type, st_name, scale_factor, info.oneWay);
                drawStreetLines(g, i, info);
            }
        }
    }

    // Draw larger streets on top in a different colour
    // Must be drawn afterwards in a separate loop
    // So that colours don't overlap in unwanted ways
    for (StreetSegmentIdx i = 0; i < segment_intersections.size(); i++) {
        StreetSegmentInfo info = getStreetSegmentInfo(i);
        std::string type = getOSMWayTagValue(info.wayOSMID, "highway");
        std::string st_name = getStreetName(info.streetID);
        if ((type == "motorway" || type == "primary" || type == "secondary") && st_name != "<unknown>"){
            ezgl::point2d med;
            med.x = segment_intersections[i].first.pos.x + ((segment_intersections[i].second.pos.x - segment_intersections[i].first.pos.x) / 2);
            med.y = segment_intersections[i].first.pos.y + ((segment_intersections[i].second.pos.y - segment_intersections[i].first.pos.y) / 2);
            if (isVisible(segment_intersections[i].first.pos, g) || isVisible(segment_intersections[i].second.pos, g) || isVisible(med, g)){
                setStreetStyle(g, type, st_name, scale_factor, false);
                drawStreetLines(g, i, info);
            }
        }
    }
}

// A helper function called in draw_main_canvas
// Draws names of streets 
void drawStreetNames(ezgl::renderer * g, double scale_factor){
    StreetSegmentInfo infoprev = getStreetSegmentInfo(getNumStreetSegments() - 1);
    for (StreetSegmentIdx i = 0; i < segment_intersections.size(); i++) {
        StreetSegmentInfo info = getStreetSegmentInfo(i);

        // Get the midpoint
        ezgl::point2d med;
        med.x = segment_intersections[i].first.pos.x + ((segment_intersections[i].second.pos.x - segment_intersections[i].first.pos.x) / 2);
        med.y = segment_intersections[i].first.pos.y + ((segment_intersections[i].second.pos.y - segment_intersections[i].first.pos.y) / 2);

        if (scale_factor < 0.012 && isVisible(med, g) == true) {
            if (getStreetName(info.streetID) != "<unknown>"  && getStreetName(infoprev.streetID)!= getStreetName(info.streetID)) {
                // Set the text colour
                if (night){
                    g -> set_color(ezgl::WHITE);
                }
                else{
                    g -> set_color(ezgl::BLACK);
                }

                // Set the text size
                g -> set_font_size(0.1/scale_factor);
                
                // Set the angle to draw the name 
                // corresponding to the angle of the street
                float strtLen = 10/scale_factor;
                double slope = getSlope(info);
                double angle = atan(slope);
                angle = angle * 180/G_PI;
                g -> set_text_rotation(angle);

                // Draw the name
                g -> draw_text(med, getStreetName(info.streetID), strtLen, strtLen);
                
            }
        }
        infoprev = info;
    }
}

// Helper function for determining angle 
// to print street names
double getSlope(StreetSegmentInfo info){
    ezgl::point2d from = intersection_data[info.from].pos;
    ezgl::point2d to = intersection_data[info.to].pos;
    double rise = to.y - from.y;
    double run = to.x - from.x;
    return rise/run;
}

// A helper function called in drawStreets
// Sets the line colour, width and other styles 
// based on information given about the street
// about to be drawn
void setStreetStyle(ezgl::renderer * g, std::string type, std::string name, double scale_factor, bool one_way){
    // Styles for smaller streets
    if (type == "tertiary" || type == "residential" || type == "unclassified" || name == "<unknown>" ){
        if (night){
            g -> set_color(77, 74, 155);
        }
        else{
            g -> set_color(217, 207, 253);
        }
            
        ezgl::line_dash dash;
        if (one_way == true){
            dash = (ezgl::line_dash) 1; // One way streets are dashed
            g -> set_line_dash(dash);
            ezgl::line_cap cap = (ezgl::line_cap) 0; // Flat ends so dashes show up at all widths
            g -> set_line_cap(cap);
        }
        else {
            dash = (ezgl::line_dash) 0;
            g -> set_line_dash(dash);
            ezgl::line_cap cap = (ezgl::line_cap) 1; // Round ends to make segments look connected
            g -> set_line_cap(cap);
        }

        if (scale_factor > 0.012){
            g -> set_line_width(0.15/scale_factor);
        }
        else{
            g -> set_line_width(0.05/scale_factor);
        }
        
    }
    // Styles for larger streets
    else if ((type == "motorway" || type == "primary" || type == "secondary") && name != "<unknown>"){
        ezgl::line_dash dash = (ezgl::line_dash) 0;
        g -> set_line_dash(dash);
        ezgl::line_cap cap = (ezgl::line_cap) 1; // Round ends to make segments look connected
        g -> set_line_cap(cap);

        if (night){
            g -> set_color(123, 83, 183);
        }
        else{
            g -> set_color(200, 150, 255);
        }

        if (type == "motorway" || type == "primary") {
            if (scale_factor > 0.012){
                g -> set_line_width(0.6/scale_factor);
            }
            else{
                g -> set_line_width(0.2/scale_factor);
            }
        } 
        else if (type == "secondary"){
            if (scale_factor > 0.012){
                g -> set_line_width(0.4/scale_factor);
            }
            else{
                g -> set_line_width(0.1/scale_factor);
            }
        } 
        else{
            if (scale_factor > 0.012){
                g -> set_line_width(0.3/scale_factor);
            }
            else{
                g -> set_line_width(0.08/scale_factor);
            }
        }
    }      
}

// A helper function called in drawStreets
// Draws the actual lines and curves given the points
// After styles have been set
void drawStreetLines(ezgl::renderer * g, int i, StreetSegmentInfo info){
    if (info.numCurvePoints > 0){
        // If the segment has curve points
        LatLon first_P = getStreetSegmentCurvePoint(0, i);
        
        // From first segment point to first curve point
        g -> draw_line({
        segment_intersections[i].first.pos.x,
        segment_intersections[i].first.pos.y
        }, {
        x_from_lon(first_P.longitude()),
        y_from_lat(first_P.latitude())
        });

        // Drawing curve points
        for (int curvP = 0; curvP < info.numCurvePoints - 1; curvP++) {
            LatLon currPoint = getStreetSegmentCurvePoint(curvP, i);
            LatLon currPoint2 = getStreetSegmentCurvePoint(curvP + 1, i);
            g -> draw_line({
            x_from_lon(currPoint.longitude()),
            y_from_lat(currPoint.latitude())
            }, {
            x_from_lon(currPoint2.longitude()),
            y_from_lat(currPoint2.latitude())
            });
        }

        LatLon lastPoint = getStreetSegmentCurvePoint(info.numCurvePoints - 1, i);

        // From last curve point to last segment point
        g -> draw_line({
        x_from_lon(lastPoint.longitude()),
        y_from_lat(lastPoint.latitude())
        }, {
        segment_intersections[i].second.pos.x,
        segment_intersections[i].second.pos.y
        });
    }
    else {
        // If segment has no curve points
        g -> draw_line({
        segment_intersections[i].first.pos.x,
        segment_intersections[i].first.pos.y
        }, {
        segment_intersections[i].second.pos.x,
        segment_intersections[i].second.pos.y
        });
    }
}

// A helper function called in draw_main_canvas
// Draws pin on all highlighted intersections
// Highlight is set by clicking on map
// Or using Intersections menu
void drawIntersections(ezgl::renderer *g){
    for (size_t i= 0; i < intersection_data.size();i++){
        //only shows intersection when clicked, highlighted
        if (intersection_data[i].highlight == true && isVisible(intersection_data[i].pos, g)){
            ezgl::point2d inter_loc = intersection_data[i].pos; 

            ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/pin.png");
            g->draw_surface(icon, inter_loc, 0.4);
        }
    }
}

// A helper function called in draw_main_canvas
// Draws icons & for Points of Interest when the 
// corresponding category is set as active (in Layers menu)
// Also draw POI pin if POI is highlighted
void drawPOIs(ezgl::renderer *g){
    float size_factor = 0.2; // Relative size of icon
    gboolean g_true = 1;
    
    for (POIIdx i = 0; i < POI_data.size(); i++){
        ezgl::point2d POI_loc = POI_data[i].pos;

        // Only draw the POI if in visible range
        if (isVisible(POI_loc, g) == true){
            // Determine icon based on POI type/categorization

            // aeroway: aerodrome
            if (getOSMWayTagValue(getPOIOSMNodeID(i), "aeroway") == "aerodrome" && POI_on[29] == g_true){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/airport.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
        
            // amenity: restaurant, veg, halal
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") =="restaurant" ){
                if (getOSMNodeTagValue(getPOIOSMNodeID(i), "diet:vegetarian") == "yes" && (POI_on[1] == g_true || POI_on[0] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/vegetarian.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
                else if(getOSMNodeTagValue(getPOIOSMNodeID(i), "diet:vegan") == "yes" && (POI_on[2] == g_true || POI_on[0] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/vegan.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
                else if(getOSMNodeTagValue(getPOIOSMNodeID(i), "diet:halal") == "yes" && (POI_on[3] == g_true || POI_on[0] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/halal.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
                else if (POI_on[0] == g_true){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/restaurant.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
            }
            
            // amenity: driving school
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") =="driving_school" && (POI_on[5] == g_true || POI_on[4] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/driving.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: language school
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "language_school" && (POI_on[6] == g_true || POI_on[4] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/language.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: library
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "library" && (POI_on[7] == g_true || POI_on[4] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/library.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: bank
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "bank" && (POI_on[9] == g_true || POI_on[8] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/bank.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: bureau de change
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "bureau_de_change" && (POI_on[10] == g_true || POI_on[8] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/exchange.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: clinic
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "clinic" && (POI_on[13] == g_true || POI_on[11] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/clinic.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: dentist
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "dentist" && (POI_on[15] == g_true || POI_on[11] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/dentist.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            
            // amenity: hospital
            if ((getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "hospital" || getOSMWayTagValue(getPOIOSMNodeID(i), "amenity") == "hospital") && (POI_on[12] == g_true || POI_on[11] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/hospital.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
                    
            // amenity: pharmacy
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "pharmacy" && (POI_on[14] == g_true || POI_on[11] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/pharmacy.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
                    
            // amenity: community_centre
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") == "community_centre" && (POI_on[27] == g_true || POI_on[26] == g_true)){
                if (getOSMNodeTagValue(getPOIOSMNodeID(i), "community_centre:for") == "immigrant"){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/community.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
            }
            
            // amenity: place_worship, religions
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") =="place_of_worship"){
                g->set_color(ezgl::BLACK);
                g->set_font_size(10);
                ezgl::point2d Print_loc;
                Print_loc.x= POI_loc.x; //+ 0.01;
                Print_loc.y= POI_loc.y - 3.5;
                float strtLen= 100;
                std:: string poiN = getPOIName(i);
                if (getOSMNodeTagValue(getPOIOSMNodeID(i), "religion") =="christian" && (POI_on[18] == g_true || POI_on[16] == g_true)){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/church.png");
                    g->draw_surface(icon, POI_loc, size_factor);
                    g->draw_text (Print_loc, poiN, strtLen, strtLen);
                    g->set_color (200, 150, 255);
                }
                else if (getOSMNodeTagValue(getPOIOSMNodeID(i), "religion") =="hindu" && (POI_on[20] == g_true || POI_on[16] == g_true)){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/htemple.png");
                    g->draw_surface(icon, POI_loc, size_factor);
                    g->draw_text (Print_loc, poiN, strtLen, strtLen);
                    g->set_color (200, 150, 255);
                }
                else if (getOSMNodeTagValue(getPOIOSMNodeID(i), "religion") =="muslim" && (POI_on[17] == g_true || POI_on[16] == g_true)){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/mosque.png"); 
                    g->draw_surface(icon, POI_loc, size_factor);
                    g->draw_text (Print_loc, poiN, strtLen, strtLen);
                    g->set_color (200, 150, 255);
                }       
                else if (getOSMNodeTagValue(getPOIOSMNodeID(i), "religion") == "sikh" && (POI_on[21] == g_true || POI_on[16] == g_true)){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/synagogue.png");
                    g->draw_surface(icon, POI_loc, size_factor);
                }           
                else if (getOSMNodeTagValue(getPOIOSMNodeID(i), "religion") == "buddhist" && (POI_on[19] == g_true || POI_on[16] == g_true)){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/btemple.png");
                    g->draw_surface(icon, POI_loc, size_factor);
                }      
                else if (POI_on[16] == g_true){
                    ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/worship.png");
                g->draw_surface(icon, POI_loc, size_factor);
                }
            }        
            
            // amenity: refugee_site
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "amenity") =="refugee_site" && (POI_on[30] == g_true || POI_on[26] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/refugee.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }        
            
            
            // boundary: office, diplomatic, employment agency, government
            if ((getOSMNodeTagValue(getPOIOSMNodeID(i), "diplomatic") == "embassy" || getOSMNodeTagValue(getPOIOSMNodeID(i), "diplomatic") == "consulate") && (POI_on[24] == g_true || POI_on[22] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/diplomatic.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "office") == "employment_agency" && (POI_on[25] == g_true || POI_on[22] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/employment.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "office") == "government" && (POI_on[23] == g_true || POI_on[22] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/government.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }                          
            

            // boundary: supermarket
            if (getOSMNodeTagValue(getPOIOSMNodeID(i), "shop") =="supermarket" && (POI_on[28] == g_true || POI_on[26] == g_true)){
                ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/supermarket.png");
                g->draw_surface(icon, POI_loc, size_factor);
            }
        }
    }  

    // Display POI pin if location is highlighted
    // Highlight is set by typing POI name into
    // the searchbar at top left
    for (POIIdx i = 0; i < POI_data.size(); i++){
        if (POI_data[i].highlight == true && isVisible(POI_data[i].pos, g) == true){
            ezgl::point2d POI_loc = POI_data[i].pos;
            
            ezgl::surface* icon = g->load_png("libstreetmap/resources/poi/POI.png");
            g->draw_surface(icon, POI_loc, 0.4);
        }
    }
}

// --------------------------- Helper Functions & Callback Functions ---------------------------

// Handles mouse click event on MainCanvas area
// Sets navigation start/end points if navigation mode is on
// Otherwise sets intersection highlight for pin to be drawn
void act_on_mouse_click(ezgl::application* app, GdkEventButton* /*event*/, double x, double y){
    intersection_data[inter_last].highlight = false;
    POI_data[POI_prev].highlight = false;
    LatLon pos = LatLon (lat_from_y(y), lon_from_x(x));
    
    int inter_id = findClosestIntersection(pos);
    
    if (nav == true){
        if (inter_first == false){
            inter_one = inter_id;
            inter_first = true;
            firstIntersection(app);
        }
        else{
            inter_two = inter_id;
            inter_first = false;
            secondIntersection(app);
        }
    }
    else{
        inter_last = inter_id;
        intersection_data[inter_id].highlight = true;
        inter_one = -1;
        inter_two = -1;
    }
    std::stringstream ss;
    ss<< "Intersection \""<< intersection_data[inter_id].name<< "\" at ("<< pos.latitude() <<","<<pos.longitude()<<")";
    app->update_message(ss.str());
    app->refresh_drawing(); // forces redraw so that highlight is updated immediatly and not after next drawCanvas when screen moved
}

// Converts longitude to cartesian x
double x_from_lon (float lon){
    return (lon * kDegreeToRadian * kEarthRadiusInMeters * std::cos(avg_lat*kDegreeToRadian));
}

// Converts latitude to cartesian y
double y_from_lat (float lat){
    return (lat* kDegreeToRadian*kEarthRadiusInMeters);
}

// Converts cartesian y to latitude
double lat_from_y(float y){
    return y / (kDegreeToRadian * kEarthRadiusInMeters);
}

// Converts cartesian x to longitude
double lon_from_x(float x){
    return x / (kDegreeToRadian * kEarthRadiusInMeters * std::cos(avg_lat*kDegreeToRadian));
}

// Initializes UI elements, connects signal callbacks, imports CSS styling
void initial_setup (ezgl::application* application, bool){
    // Signal callbacks
    GObject *help_button = application->get_object("HelpButton");
    GObject *night_button = application->get_object("NightButton");
    GObject *map_button = application->get_object("LoadMapButton");
    GObject *intersection_button = application->get_object("IntersectionButton");
    GObject *nav_button = application->get_object("NavigationButton");
    GObject *language_button = application->get_object("LanguageButton");
    GObject *poi_button = application->get_object("LayerButton");
    GObject *submit_button = application->get_object("InterSubmit");
    GObject *submit_nav = application->get_object("NavSubmit");
    GObject *submit_nav_2 = application->get_object("NavSubmit2");
    GObject *search_entry = application->get_object("SearchBar");
    GObject *poi_boxes = application->get_object("List");
    
    g_signal_connect(help_button, "clicked", G_CALLBACK(helpDoc), application);
    g_signal_connect(night_button, "clicked", G_CALLBACK(nightMode), application);
    g_signal_connect(map_button, "clicked", G_CALLBACK(chooseMap), application);
    g_signal_connect(intersection_button, "clicked", G_CALLBACK(findIntersections), application);
    g_signal_connect(language_button, "clicked", G_CALLBACK(changeLang), application);
    g_signal_connect(nav_button, "clicked", G_CALLBACK(navMenu), application);
    g_signal_connect(poi_button, "clicked", G_CALLBACK(layerToggle), application);
    g_signal_connect(submit_button, "clicked", G_CALLBACK(twoIntersections), application);
    g_signal_connect(submit_nav, "clicked", G_CALLBACK(submitNav), application);
    g_signal_connect(submit_nav_2, "clicked", G_CALLBACK(submitNav2), application);
    g_signal_connect(search_entry, "activate", G_CALLBACK(enterSearch), application);
    g_signal_connect(poi_boxes, "button-release-event", G_CALLBACK(poiToggle), application);

    GObject *map_list = application->get_object("MapList");
    g_signal_connect(map_list, "changed", G_CALLBACK(changeMap), application);

    GObject *lang_list = application->get_object("LangList");
    g_signal_connect(lang_list, "changed", G_CALLBACK(langChoice), application);

    // Initial status bar message
    application->update_message("Click anywhere on the map to view the nearest intersection");
    
    // Load CSS style sheet
    loadCss("libstreetmap/src/style.css");

    // Fill screen with canvas according to screen dimensions
    auto canvas = application->get_canvas("MainCanvas");
    ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());
}

// Checks whether given point is in visible range
// Used as helper function for drawing map elements
bool isVisible(ezgl::point2d point, ezgl::renderer* g){
    ezgl::rectangle screen = g -> get_visible_world();
    double point_x = point.x;
    double point_y = point.y;
    double min_x = screen.m_first.x;
    double min_y = screen.m_first.y;
    double max_x = screen.m_second.x;
    double max_y = screen.m_second.y;

    bool min = point_x >= min_x && point_y >= min_y;
    bool max = point_x <= max_x && point_y <= max_y;

    return min && max;
}

// --------------------------- UI Related Helper & Callback Functions --------------------------

// Helper function to load and apply
// a CSS style sheet to UI elements
void loadCss(const gchar* path){
    std::ifstream f(path);
    if (f.good()){
        GtkCssProvider* css = gtk_css_provider_new();
        gtk_css_provider_load_from_path(css, path, NULL); 

        GdkDisplay *display = gdk_display_get_default();
        GdkScreen *screen = gdk_display_get_default_screen(display);
        gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

        g_object_unref(css);
    }
}

// Callback function
// Toggles Layer menu when Layer button pressed
void layerToggle(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("Layers", application);
}

// Callback function
// Toggles Help menu when Help button pressed
void helpDoc(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("Help", application);
}

// Callback function
// Toggles Navigation menu when Navigation button pressed
void navMenu(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("Navigation", application);
}

// Callback function
// Validates input when entering starting nav point
// If valid, records input and opens menu for entering destination point
void submitNav(GtkButton* /*submit*/, ezgl::application* application){
    // Get entry widgets & label widget
    auto entry1 = application->find_widget("Inter1S1");
    auto entry2 = application->find_widget("Inter1S2");
    auto out = application->find_widget("OutputText1");
    GtkEntry* street1 = (GtkEntry*) entry1;
    GtkEntry* street2 = (GtkEntry*) entry2;
    GtkLabel* label = (GtkLabel*) out;
    
    // Get input strings
    const gchar* st1 = gtk_entry_get_text(street1);
    const gchar* st2 = gtk_entry_get_text(street2);
    std::string s1 = (std::string) st1;
    std::string s2 = (std::string) st2;

    const gchar* msg;

    if (s1 == "" || s2 == ""){
        // Invalid input 
        msg = "Please make sure to enter both\nintersection names \nbefore clicking submit.";
        const gchar* text = g_strcompress(msg);
        gtk_label_set_text(label, text);
    }
    else{
        std::vector<IntersectionIdx> common_intersections;
        std::vector<StreetIdx> streets1 = findStreetIdsFromPartialStreetName(st1);
        std::vector<StreetIdx> streets2 = findStreetIdsFromPartialStreetName(st2);

        if (streets1.size() >= 1 && streets2.size() >= 1){
            // Valid input
            common_intersections = findIntersectionsOfTwoStreets(std::make_pair(streets1[0], streets2[0]));
            if (common_intersections.size() >= 1){
                inter_one = common_intersections[0];
                inter_first = true;
                firstIntersection(application);
            }
            else{
                msg = "Could not find a unique intersection \nmatching the streets you've entered. \nPlease try again.";
                const gchar* text = g_strcompress(msg);
                gtk_label_set_text(label, text);
            }
        }
        else{
            // Valid input, but no unique match
            msg = "Could not find a street matching \none or more of the names you entered. \nPlease try again.";
            const gchar* text = g_strcompress(msg);
            gtk_label_set_text(label, text);
        }

        common_intersections.clear();
        streets1.clear();
        streets2.clear();
    }
    application->refresh_drawing();
}

// Callback function
// Validates input when entering destination nav point
// If valid, records input and opens menu with directions listed
void submitNav2(GtkButton* /*submit*/, ezgl::application* application){
    // Get entry widgets & label widget
    auto entry1 = application->find_widget("Inter2S1");
    auto entry2 = application->find_widget("Inter2S2");
    auto out = application->find_widget("OutputText2");
    GtkEntry* street1 = (GtkEntry*) entry1;
    GtkEntry* street2 = (GtkEntry*) entry2;
    GtkLabel* label = (GtkLabel*) out;
    
    // Get input strings
    const gchar* st1 = gtk_entry_get_text(street1);
    const gchar* st2 = gtk_entry_get_text(street2);
    std::string s1 = (std::string) st1;
    std::string s2 = (std::string) st2;

    const gchar* msg;

    if (s1 == "" || s2 == ""){
        // Invalid input
        msg = "Please make sure to enter both\nintersection names \nbefore clicking submit.";
        const gchar* text = g_strcompress(msg);
        gtk_label_set_text(label, text);
    }
    else{
        std::vector<IntersectionIdx> common_intersections;
        std::vector<StreetIdx> streets1 = findStreetIdsFromPartialStreetName(st1);
        std::vector<StreetIdx> streets2 = findStreetIdsFromPartialStreetName(st2);

        if (streets1.size() >= 1 && streets2.size() >= 1){
            // Valid input
            common_intersections = findIntersectionsOfTwoStreets(std::make_pair(streets1[0], streets2[0]));
            if (common_intersections.size() >= 1){
                inter_two = common_intersections[0];
                inter_first = false;
                secondIntersection(application);
            }
            else{
                // Valid input but no unique match
                msg = "Could not find a unique intersection \nmatching the streets you've entered. \nPlease try again.";
                const gchar* text = g_strcompress(msg);
                gtk_label_set_text(label, text);
            }
        }
        else{
            msg = "Could not find a street matching \none or more of the names you entered. \nPlease try again.";
            const gchar* text = g_strcompress(msg);
            gtk_label_set_text(label, text);
        }

        common_intersections.clear();
        streets1.clear();
        streets2.clear();
    }
    application->refresh_drawing();
}

// Callback function
// Toggles night mode when Night Mode button pressed
void nightMode(GtkButton* /*menu*/, ezgl::application* application){
    night = !night;

    if (night){
        loadCss("libstreetmap/src/night.css");
    }
    else{
        loadCss("libstreetmap/src/style.css");
    }
    application->refresh_drawing();
}

// Callback function
// When Layer menu recieves click event,
// checks which category buttons are active
// and records for use when drawing POI
// icons on map
void poiToggle(GtkWidget* self, GdkEventButton /*event*/, gpointer /*data*/, ezgl::application* application){
    // Getting POI list bools
    GtkContainer* container = (GtkContainer*) self;
    GList* box_list = gtk_container_get_children(container);
    for (int i = 0; i < num_POI_categories; i++){
        GtkToggleButton* check_box = (GtkToggleButton*) box_list->data;
        POI_on[i] = gtk_toggle_button_get_active(check_box);
        box_list = box_list->next;
    }

    application->refresh_drawing();
}

// Callback function
// Toggles Load Map menu when Load Map button pressed
void chooseMap(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("LoadMap", application);
}

// Callback function
// Toggles Intersections menu when Intersections button pressed
void findIntersections(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("Intersections", application);
}

// Helper function that changes the visible stack child
// to a menu given its name. If the requested menu
// is currently active already, turns the whole menu off
// Also determines whether navigation mode is on or not
// Based on whether the nav menu is currently active
void menuPopup(const gchar* layer, ezgl::application* application){
    GtkWidget* menu = application->find_widget("Menu");
    GtkWidget* menu_bg = application->find_widget("MenuBG");
    GtkStack* stack = (GtkStack*) menu;

    const gchar* vis_layer = gtk_stack_get_visible_child_name(stack);
    std::string layer_check_1 = (std::string) vis_layer;
    std::string layer_check_2 = (std::string) layer;

    if (layer_check_2 == "LoadMap"){
        gtk_widget_set_size_request (menu_bg, 300, 150);
    }
    else if (layer_check_2 == "Language"){
        gtk_widget_set_size_request (menu_bg, 300, 180);
    }
    else if (layer_check_2 == "Intersections"){
        gtk_widget_set_size_request (menu_bg, 300, 450);
    }
    else if (layer_check_2 == "Navigation" || layer_check_2 == "Navigation2"){
        gtk_widget_set_size_request (menu_bg, 300, 400);
    }
    else{
        gtk_widget_set_size_request (menu_bg, 300, 600);
    }


    if (layer_check_1 == layer_check_2){
        gtk_widget_set_visible(menu, !gtk_widget_is_visible(menu));
        gtk_widget_set_visible(menu_bg, !gtk_widget_is_visible(menu_bg));
    }
    else{
        gtk_stack_set_visible_child_name(stack, layer);
        gtk_widget_set_visible(menu, true);
        gtk_widget_set_visible(menu_bg, true);
    }

    if ((layer_check_2 == "Navigation" || layer_check_2 == "Navigation2" || layer_check_2 == "Directions") && gtk_widget_is_visible(menu)){
        nav = true;
        intersection_data[inter_last].highlight = false;
        POI_data[POI_prev].highlight = false;
        if (layer_check_2 == "Directions" || layer_check_2 == "Navigation"){
            directions = true;
        }
        else{
            directions = false;
        }
    }
    else{
        nav = false;
    }
    
    application->refresh_drawing();
}

// Helper function for Intersections menu
// Validates input, and processes if valid
void twoIntersections(GtkButton* /*button*/, ezgl::application* application){
    // Get entry widgets & label widget
    auto entry1 = application->find_widget("Street1");
    auto entry2 = application->find_widget("Street2");
    auto out = application->find_widget("OutputText");
    GtkEntry* street1 = (GtkEntry*) entry1;
    GtkEntry* street2 = (GtkEntry*) entry2;
    GtkLabel* label = (GtkLabel*) out;
    
    // Get input strings
    const gchar* st1 = gtk_entry_get_text(street1);
    const gchar* st2 = gtk_entry_get_text(street2);
    std::string s1 = (std::string) st1;
    std::string s2 = (std::string) st2;

    const gchar* msg;

    if (s1 == "" || s2 == ""){
        // Invalid input
        msg = "Please make sure to enter both\nintersection names \nbefore clicking submit.";
    }
    else{
        std::vector<IntersectionIdx> common_intersections;
        std::vector<StreetIdx> streets1 = findStreetIdsFromPartialStreetName(st1);
        std::vector<StreetIdx> streets2 = findStreetIdsFromPartialStreetName(st2);

        if (streets1.size() >= 1 && streets2.size() >= 1){
            // Valid input
            POI_data[POI_prev].highlight = false;
            intersection_data[inter_last].highlight=false;
            common_intersections = findIntersectionsOfTwoStreets(std::make_pair(streets1[0], streets2[0]));
            msg = "Here are the intersections \nthese streets have in common:\n";
            
            int end= common_intersections.size();
        
            for (int i = 0; i < end; i++){
                const gchar* name = intersection_data[common_intersections[i]].name.c_str();
                intersection_data[common_intersections[i]].highlight = true;
                inter_last = common_intersections[i];
            
                msg = g_strconcat(msg, name, "\n", NULL);
            }
        }
        else{
            // Valid input but no unique match
            msg = "Could not find a unique street matching \none or more of the names you entered. \nPlease try again.";
        }

        common_intersections.clear();
        streets1.clear();
        streets2.clear();
    }

    const gchar* text = g_strcompress(msg);

    gtk_label_set_text(label, text);

    application->refresh_drawing();
}

// Callback function
// Loads the selected map when the combobox option is changed
void changeMap(GtkComboBox* box, ezgl::application* application){
    GtkComboBoxText* combo_box = (GtkComboBoxText*) box;
    gchar* selection = gtk_combo_box_text_get_active_text(combo_box);
    std::string choice = (std::string) selection;
    std::cout << "Loading map of " << choice  << ". Please wait a few moments." << std::endl;

    closeMap();
    bool load_success = false;

    if (choice == "Beijing"){
        load_success = loadMap("/cad2/ece297s/public/maps/beijing_china.streets.bin");
    }
    else if (choice == "Boston"){
        load_success = loadMap("/cad2/ece297s/public/maps/beijing_china.streets.bin");
    }
    else if (choice == "Cape Town"){
        load_success = loadMap("/cad2/ece297s/public/maps/cape-town_south-africa.streets.bin");
    }
    else if (choice == "Golden Horseshoe"){
        load_success = loadMap("/cad2/ece297s/public/maps/golden-horseshoe_canada.streets.bin");
    }
    else if (choice == "Hamilton"){
        load_success = loadMap("/cad2/ece297s/public/maps/hamilton_canada.streets.bin");
    }
    else if (choice == "Hong Kong"){
        load_success = loadMap("/cad2/ece297s/public/maps/hong-kong_china.streets.bin");
    }
    else if (choice == "Iceland"){
        load_success = loadMap("/cad2/ece297s/public/maps/iceland.streets.bin");
    }
    else if (choice == "Interlaken"){
        load_success = loadMap("/cad2/ece297s/public/maps/interlaken_switzerland.streets.bin");
    }
    else if (choice == "Kyiv"){
        load_success = loadMap("/cad2/ece297s/public/maps/kyiv_ukraine.streets.bin");
    }
    else if (choice == "London"){
        load_success = loadMap("/cad2/ece297s/public/maps/london_england.streets.bin");
    }
    else if (choice == "New Delhi"){
        load_success = loadMap("/cad2/ece297s/public/maps/new-delhi_india.streets.bin");
    }
    else if (choice == "New York"){
        load_success = loadMap("/cad2/ece297s/public/maps/new-york_usa.streets.bin");
    }
    else if (choice == "Rio de Janeiro"){
        load_success = loadMap("/cad2/ece297s/public/maps/rio-de-janeiro_brazil.streets.bin");
    }
    else if (choice == "Saint Helena"){
        load_success = loadMap("/cad2/ece297s/public/maps/saint-helena.streets.bin");
    }
    else if (choice == "Singapore"){
        load_success = loadMap("/cad2/ece297s/public/maps/singapore.streets.bin");
    }
    else if (choice == "Sydney"){
        load_success = loadMap("/cad2/ece297s/public/maps/sydney_australia.streets.bin");
    }
    else if (choice == "Tehran"){
        load_success = loadMap("/cad2/ece297s/public/maps/tehran_iran.streets.bin");
    }
    else if (choice == "Tokyo"){
        load_success = loadMap("/cad2/ece297s/public/maps/tokyo_japan.streets.bin");
    }
    else if (choice == "Toronto"){
        load_success = loadMap("/cad2/ece297s/public/maps/toronto_canada.streets.bin");
    }

    if (load_success){ 
        refreshMap(application); 
        auto canvas = application->get_canvas("MainCanvas");
        ezgl::zoom_fit(canvas, canvas->get_camera().get_initial_world());
    }
}

// Callback function
// Toggles Language menu when Language button pressed
void changeLang(GtkButton* /*menu*/, ezgl::application* application){
    menuPopup("Language", application);
}

// Callback function
// Loads the selected language text when the combobox option is changed
void langChoice(GtkComboBox* box, ezgl::application* application){
    GtkComboBoxText* combo_box = (GtkComboBoxText*) box;
    gchar* selection = gtk_combo_box_text_get_active_text(combo_box);
    std::string choice = (std::string) selection;

    GtkToolButton* map_button = (GtkToolButton*) application->get_object("LoadMapButton");
    GtkToolButton* intersection_button = (GtkToolButton*) application->get_object("IntersectionButton");
    GtkToolButton* layer_button = (GtkToolButton*) application->get_object("LayerButton");
    GtkToolButton* navigation_button = (GtkToolButton*) application->get_object("NavigationButton");
    GtkToolButton* lang_button = (GtkToolButton*) application->get_object("LanguageButton");
    GtkToolButton* night_button = (GtkToolButton*) application->get_object("NightButton");
    GtkToolButton* help_button = (GtkToolButton*) application->get_object("HelpButton");
    GtkToolButton* proceed_button = (GtkToolButton*) application->get_object("ProceedButton");
    
    if (choice == "English"){
        gtk_tool_button_set_label(map_button, "Load Map");
        gtk_tool_button_set_label(intersection_button, "Intersections");
        gtk_tool_button_set_label(layer_button, "Layers");
        gtk_tool_button_set_label(navigation_button, "Navigation");
        gtk_tool_button_set_label(night_button, "Night Mode");
        gtk_tool_button_set_label(lang_button, "Language");
        gtk_tool_button_set_label(help_button, "Help");
        gtk_tool_button_set_label(proceed_button, "Exit");
    }
    else if (choice == "Bangla"){
        gtk_tool_button_set_label(map_button, "মানচিত্র");
        gtk_tool_button_set_label(intersection_button, "প্রতিচ্ছেদ");
        gtk_tool_button_set_label(layer_button, "জায়গা");
        gtk_tool_button_set_label(navigation_button, "দিকনির্দেশ");
        gtk_tool_button_set_label(night_button, "রাত মোড");
        gtk_tool_button_set_label(lang_button, "ভাষা");
        gtk_tool_button_set_label(help_button, "সাহায্য");
        gtk_tool_button_set_label(proceed_button, "বন্ধ");
    }
    else if (choice == "Urdu"){
        gtk_tool_button_set_label(map_button, "نقشہ");
        gtk_tool_button_set_label(intersection_button, "چوراہا");
        gtk_tool_button_set_label(layer_button, "مقام");
        gtk_tool_button_set_label(navigation_button, "سمت شناسی");
        gtk_tool_button_set_label(night_button, "نائٹ موڈ");
        gtk_tool_button_set_label(lang_button, "زبان");
        gtk_tool_button_set_label(help_button, "مدد");
        gtk_tool_button_set_label(proceed_button, "بند");
    }
    else if (choice == "Hindi"){
        gtk_tool_button_set_label(map_button, "नक्शा");
        gtk_tool_button_set_label(intersection_button, "चौराहा");
        gtk_tool_button_set_label(layer_button, "जगह");
        gtk_tool_button_set_label(navigation_button, "मार्गदर्शन");
        gtk_tool_button_set_label(night_button, "रात का मोड");
        gtk_tool_button_set_label(lang_button, "भाषा");
        gtk_tool_button_set_label(help_button, "मदद");
        gtk_tool_button_set_label(proceed_button, "बंद");
    }
}

// Callback function
// Reads search bar text when enter key is pressed
// Processes input to find a matching POI, then sets its
// highlight to true for the pin to be drawn
// Not case sensitive, but doesn't work with partial names
void enterSearch(GtkEntry* search_bar, ezgl::application* application) {
    const gchar *search_text = gtk_entry_get_text(search_bar);
    double x = 0;
    double y = 0;
    bool found = false;
    // Making it so it's not case sensitive
    std::string check_1 = toLowerRemoveSpace((std::string) search_text);
    std::string check_2;
    std::string found_text; 

    for (POIIdx i = 0; i < POI_data.size(); i++){
        // Making it so it's not case sensitive
        check_2 = toLowerRemoveSpace(POI_data[i].name);

        if (check_1 == check_2){
            POI_data[POI_prev].highlight = false;
            POI_data[i].highlight = true;
            intersection_data[inter_last].highlight = false;
            application->refresh_drawing();
            POI_prev = i;
            found = true;
            x = POI_data[i].pos.x;
            y = POI_data[i].pos.y;
            found_text = POI_data[i].name;
        }
    }

    std::string msg;
    if (!found){
        msg = "The POI you have entered was not found";
    }
    else{
        std::stringstream s;
        s << (std::string) found_text << " found at (" << x << ", " << y << ")";
        msg = s.str();
    }
    application->update_message(msg);
}

// --------------------------- M3 Related Helper & Callback Functions --------------------------

// Helper function that draws navigation path onto the map 
void drawPath(std::vector<StreetSegmentIdx> segments, ezgl::renderer* g, double scale_factor){
    for (auto it = segments.begin(); it != segments.end(); it++){
        ezgl::point2d med;
        med.x = segment_intersections[*it].first.pos.x + ((segment_intersections[*it].second.pos.x - segment_intersections[*it].first.pos.x) / 2);
        med.y = segment_intersections[*it].first.pos.y + ((segment_intersections[*it].second.pos.y - segment_intersections[*it].first.pos.y) / 2);
        if (directions == true && (isVisible(segment_intersections[*it].first.pos, g) || isVisible(segment_intersections[*it].second.pos, g) || isVisible(med, g))){
            StreetSegmentInfo info = getStreetSegmentInfo(*it);
            std::string type = getOSMWayTagValue(info.wayOSMID, "highway");
            std::string st_name = getStreetName(info.streetID);
            
            ezgl::line_dash dash = (ezgl::line_dash) 0;
            g -> set_line_dash(dash);
            ezgl::line_cap cap = (ezgl::line_cap) 1; // Round ends to make segments look connected
            g -> set_line_cap(cap);
         
            if (scale_factor > 0.012){
                g -> set_line_width(0.6/scale_factor);
            }
            else{
                g -> set_line_width(0.2/scale_factor);
            }

            // setStreetStyle(g, type, st_name, scale_factor, info.oneWay);
            g -> set_color(255, 123, 118);
            drawStreetLines(g, *it, info);
        }
    }
}

// Helper function that creates a button and label 
// corresponding to a direction and adds it to the
// list of directions (implented with a grid)
void addDirection(const gchar* text, int dir, int row, ezgl::application* application){
    GtkWidget *new_dir = gtk_label_new_with_mnemonic(text);
    GtkStyleContext *context = gtk_widget_get_style_context(new_dir);
    gtk_style_context_add_class(context,"navigation");
    gtk_label_set_max_width_chars ((GtkLabel*) new_dir, 25);
    gtk_label_set_line_wrap ((GtkLabel*) new_dir, TRUE);
    gtk_widget_set_halign(new_dir, GTK_ALIGN_START);
    gtk_label_set_xalign ((GtkLabel*) new_dir, 0);

    GtkWidget* icon = NULL;

    if (dir == 0){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/forward.svg");
    }
    else if(dir == 1){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/right.svg");
    }
    else if (dir == 2){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/left.svg");
    }
    else if(dir == 3){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/slight right.svg");
    }
    else if(dir == 4){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/slight left.svg");
    }
    else if (dir == -1925){
        icon = gtk_image_new_from_file("libstreetmap/resources/directions/destination.svg");
    }
    
    context = gtk_widget_get_style_context(icon);
    gtk_style_context_add_class(context,"directions");
    gtk_widget_set_halign(icon, GTK_ALIGN_START);

    GtkGrid *queue = (GtkGrid *) application->find_widget("Queue");
    gtk_grid_insert_row(queue, row);
    gtk_grid_attach(queue, icon, 0, row, 1, 1);
    gtk_grid_attach(queue, new_dir, 1, row, 1, 1);
    gtk_widget_show(new_dir);
    gtk_widget_show(icon);
}

// Helper function that opens nav menu for next input point
void firstIntersection(ezgl::application* application){
    menuPopup("Navigation2", application);
}

// Helper function that processes nav inputs to find
// path between them, and determines directions for that path
void secondIntersection(ezgl::application* application){
    if (inter_one != NO_DATA && inter_two != NO_DATA){
        nav_segments = findPathBetweenIntersections(5, std::make_pair(inter_one, inter_two));
    }

    if (num_directions != 0){
        deleteDirections(application);
    }
    addDirection("Destination Reached", -1925, 1, application);
    int row = 1;
    double meters = 0;
    IntersectionIdx turn = -1;
    const gchar* text;
 
    for (auto it = nav_segments.begin(); it != nav_segments.end(); it++){
        auto it2 = it + 1; 

        if (it2 != nav_segments.end()){
            StreetSegmentInfo src_info = getStreetSegmentInfo(*it);
            StreetSegmentInfo dest_info = getStreetSegmentInfo(*it2);
            std::string src = getStreetName(src_info.streetID);
            std::string dest = getStreetName(dest_info.streetID);
        
            if (src_info.streetID == dest_info.streetID){
                meters += findStreetSegmentLength(*it);
            }
            else{
                meters += findStreetSegmentLength(*it);
                if (src_info.to == dest_info.from || src_info.to == dest_info.to){
                    turn = src_info.to;
                }
                else {
                    turn = src_info.from;
                }

                std::stringstream ss;
                ss << "Continue on " << src << " for " << std::ceil(meters) << " m";
                std::string msg = ss.str();
                text = msg.c_str();
                addDirection(text, 0, row, application);
                row++;
                
                int dir = findDirection(*it, *it2);
                if (dir == 0){
                    text = "Continue forward at ";
                }
                else if(dir == 1){
                    text = "Turn right on ";
                }
                else if(dir == 3){
                    text = "Turn right on ";
                }
                else if(dir == 2){
                    text = "Turn left on ";
                }
                else {
                    text = "Turn left on ";
                }
                text = g_strconcat(text, intersection_data[turn].name.c_str(), NULL);
                addDirection(text, dir, row, application);
                meters = 0;
                row++;
            }
        }
        else{
            StreetSegmentInfo src_info = getStreetSegmentInfo(*it);
            std::string src = getStreetName(src_info.streetID);
            meters += findStreetLength(src_info.streetID);

            std::stringstream ss;
            ss << "Continue on " << src << " for " << std::ceil(meters) << " m";
            std::string msg = ss.str();
            text = msg.c_str();
            addDirection(text, 0, row, application);
            row++;
        }
    }
    num_directions = row; 
    menuPopup("Directions", application);
}

// Helper function to delete previous direction list 
// Before directions for a new path are added
void deleteDirections(ezgl::application* application){
    GtkWidget* queue = application->find_widget("Queue");
    GtkGrid* grid = (GtkGrid*) queue;
    for (int i = 1; i <= num_directions; i++){
        GtkWidget* direction = gtk_grid_get_child_at(grid, 1, i);
        GtkWidget* image = gtk_grid_get_child_at(grid, 0, i);
        gtk_widget_destroy(direction);
        gtk_widget_destroy(image);
    }
}

// Helper function to determine angle for direction icon and text
// Returns 1 for right, 2 for left, 3 for slight right, 4 for slight left, 0 for completely straight ahead
int findDirection(StreetSegmentIdx src, StreetSegmentIdx dest){
    StreetSegmentInfo src_info = getStreetSegmentInfo(src);
    StreetSegmentInfo dest_info = getStreetSegmentInfo(dest);

    ezgl::point2d src_from = intersection_data[src_info.from].pos;
    ezgl::point2d src_to = intersection_data[src_info.to].pos;
    ezgl::point2d dest_from = intersection_data[dest_info.from].pos;
    ezgl::point2d dest_to = intersection_data[dest_info.to].pos;
    double src_x, src_y, dest_x, dest_y;

    if (src_info.to == dest_info.to){
        src_x = src_to.x - src_from.x;
        src_y = src_to.y - src_from.y;
        dest_x = -(dest_to.x - dest_from.x);
        dest_y = -(dest_to.y - dest_from.y);
    }
    else if (src_info.to == dest_info.from){
        src_x = src_to.x - src_from.x;
        src_y = src_to.y - src_from.y;
        dest_x = dest_to.x - dest_from.x;
        dest_y = dest_to.y - dest_from.y;
    }
    else if (src_info.from == dest_info.to){
        src_x = -(src_to.x - src_from.x);
        src_y = -(src_to.y - src_from.y);
        dest_x = -(dest_to.x - dest_from.x);
        dest_y = -(dest_to.y - dest_from.y);
    }
    else {
        src_x = -(src_to.x - src_from.x);
        src_y = -(src_to.y - src_from.y);
        dest_x = dest_to.x - dest_from.x;
        dest_y = dest_to.y - dest_from.y;
    }

    double sign = src_x*dest_y - src_y*dest_x;
    double angle = findAngleBetweenStreetSegments(src, dest);

    if (sign < 0 and angle > 1){
        return 1;
    }
    else if (sign > 0 and angle > 1){
        return 2;
    }
    else if (sign < 0 and angle <= 1){
        return 3;
    }
    else if (sign > 0 and angle <= 1){
        return 4;
    }

    return 0; // If none of the above statements causes a return, then angle == 0 so return 0 for forward
}