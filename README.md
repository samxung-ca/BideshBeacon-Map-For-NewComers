# BideshBeacon-Map-For-NewComers
BideshBeacon makes South Asian internationals feel at home, being a newcomer centered map application that highlights important locations and resources in different cities. Using the OSM Database API it provides key location info while using GTK for the GUI, made in C++.

Users can type locations they're looking for in the search bar, with a drop down menu of cities and points of interests to choose from. The icons transcend language barriers, allowing for universally understood signals. This map offers curated Point of Interest layers, with directions and support in multiple languages.

Note: This project was made for coursework, hence much of the source code is not made public.

https://github.com/user-attachments/assets/90d40ba8-f433-43ce-8a09-77a5d25e8bed


Using Dijkstras algorithm the shortest path is calculated. 


https://github.com/user-attachments/assets/350065bc-8c2c-493e-91a9-c2e4ed93513b


Route finding heuristics: Runs greedy algorithm multiple times with different start locations, uses shift perturbations to further improve routes, and used simulated annealing with perturbations to allow Hill Climbing.

Selective drawing (details depending on zoom levels); Features are drawn selectively based on zoom levels. This prioritizes performance by drawing features after certain thresholds. This enhances user experience with faster response times and smoother navigation.


https://github.com/user-attachments/assets/3878c760-c4b1-4384-b9df-5697d8b71e3b


All elements (features, streets, POIs, text etc) are only drawn if within user’s visible window. It eliminates unnecessary draw time spent on information not viewed by user
and improves frame refresh rates when user is panning and zooming. 

Language features are displayed: 

https://github.com/user-attachments/assets/f37f8687-1c7c-488d-9366-57431158539e



