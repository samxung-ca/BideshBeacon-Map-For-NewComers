# BideshBeacon-Map-For-NewComers
BideshBeacon makes internationals feel at home, being a newcomer centered map application that highlights important locations and resources in different cities. Using the OSM Database API it provides key location info while using GTK for the GUI, made in C++.

This map offers curated Point of Interest layers, with directions and support in multiple languages.


https://github.com/user-attachments/assets/90d40ba8-f433-43ce-8a09-77a5d25e8bed


Using Dijkstras algorithm the shortest path is calculated. 


https://github.com/user-attachments/assets/350065bc-8c2c-493e-91a9-c2e4ed93513b


Route finding heuristics: Runs greedy algorithm multiple times with different start locations, uses shift perturbations to further improve routes, and used simulated annealing with perturbations to allow Hill Climbing.

Selective drawing (details depending on zoom levels); Features are drawn selectively based on zoom levels. This prioritizes performance by drawing features after certain thresholds. This enhances user experience with faster response times and smoother navigation.

All elements (features, streets, POIs, text etc) are only drawn if within user’s visible window. It eliminates unnecessary draw time spent on information not viewed by user
and improves frame refresh rates when user is panning and zooming. 


