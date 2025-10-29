#include "map_generator.h"
#include "live_reload.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>

void generateMap(const nlohmann::ordered_json &configJson, const std::string &icao, bool openBrowser)
{
    std::string filename = std::filesystem::path("configs/").string() + icao + "_map.html";
    std::ofstream htmlFile(filename);

    if (!htmlFile)
    {
        std::cout << RED << "Error creating HTML file." << RESET << std::endl;
        return;
    }

    // Calculate bounds from all stands so we can fit the map to show them all
    double totalLat = 0, totalLon = 0;
    double centerLat = 0, centerLon = 0;
    int zoomLevel = 6;
    int validStands = 0;
    double minLat = 1e9, maxLat = -1e9, minLon = 1e9, maxLon = -1e9;
    double firstLat = 0, firstLon = 0;

    if (!configJson.contains("Stands") || !configJson["Stands"].is_object())
    {
        centerLat = 47.009279;
        centerLon = 3.765732;
    }
    else
    {
        for (auto &[standName, standData] : configJson["Stands"].items())
        {
            if (standData.contains("Coordinates"))
            {
                std::string coords = standData["Coordinates"];
                size_t firstColon = coords.find(':');
                size_t secondColon = coords.find(':', firstColon + 1);
                if (firstColon != std::string::npos && secondColon != std::string::npos)
                {
                    try
                    {
                        double lat = std::stod(coords.substr(0, firstColon));
                        double lon = std::stod(coords.substr(firstColon + 1, secondColon - firstColon - 1));
                        totalLat += lat;
                        totalLon += lon;
                        // update bounds
                        if (lat < minLat) minLat = lat;
                        if (lat > maxLat) maxLat = lat;
                        if (lon < minLon) minLon = lon;
                        if (lon > maxLon) maxLon = lon;
                        if (validStands == 0) { firstLat = lat; firstLon = lon; }
                        validStands++;
                    }
                    catch (...)
                    { /* Skip invalid coordinates */
                    }
                }
            }
        }

    centerLat = validStands > 0 ? totalLat / validStands : 47.009279;
    centerLon = validStands > 0 ? totalLon / validStands : 3.765732;
        // (Stand JS generation moved into the HTML <script> block below.)
        // Generate timestamp for JavaScript use
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();

        htmlFile << R"(<!DOCTYPE html>
<html>
<head>
    <title>)" << icao
                 << R"( - Airport Stands Debug Map</title>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <style>
        #map { height: 100vh; width: 100%; }
        .stand-info { font-weight: bold; }
        .legend { 
            background: white; 
            padding: 10px; 
            border-radius: 5px; 
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
    </style>
</head>
<body>
    <div id="map"></div>
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script>
        var map = L.map('map', {
            maxZoom: 19  // Increase maximum zoom level
        }).setView([)"
                 << centerLat << ", " << centerLon << R"(], )" << zoomLevel << R"();
        
        // Add satellite tile layer
        L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
            attribution: 'Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, and the GIS User Community',
            maxZoom: 19  // Set tile layer max zoom
        }).addTo(map);
        
        // Add OpenStreetMap overlay for airport details
        var osmLayer = L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
            opacity: 0.3,
            maxZoom: 19  // OSM has lower max zoom
        }).addTo(map);
        
        // Store references to current stands for cleanup
        var currentStandElements = [];
        
        // Color function for different stand types
        function getStandColor(standData) {
            if (standData.Apron) return '#FF6B6B';  // Red for apron
            if (standData.Schengen === false) return '#4ECDC4';  // Teal for non-Schengen
            if (standData.Schengen === true) return '#45B7D1';   // Blue for Schengen
            return '#96CEB4';  // Green for default
        }
        
        function getStandOpacity(standData) {
            return standData.Use ? 0.4 : 0.2;
        }
)";

        // Add stands to the map
        if (configJson.contains("Stands") && configJson["Stands"].is_object())
        {
            for (auto &[standName, standData] : configJson["Stands"].items())
            {
                if (standData.contains("Coordinates"))
                {
                    std::string coords = standData["Coordinates"];
                    size_t firstColon = coords.find(':');
                    size_t secondColon = coords.find(':', firstColon + 1);
                    if (firstColon != std::string::npos && secondColon != std::string::npos)
                    {
                        try
                        {
                            double lat = std::stod(coords.substr(0, firstColon));
                            double lon = std::stod(coords.substr(firstColon + 1, secondColon - firstColon - 1));
                            std::string radiusStr = coords.substr(secondColon + 1);
                            double radius = radiusStr.empty() ? 20 : std::stod(radiusStr);
                            std::string standNameVar = standName;
                            std::replace(standNameVar.begin(), standNameVar.end(), ' ', '_');

                            htmlFile << "        // Stand " << standName << "\n";
                            htmlFile << "        var stand_" << standNameVar << " = {\n";
                            htmlFile << "            name: '" << standName << "',\n";
                            htmlFile << "            lat: " << lat << ",\n";
                            htmlFile << "            lon: " << lon << ",\n";
                            htmlFile << "            radius: " << radius << ",\n";
                            if (standData.contains("Code"))
                                htmlFile << "            code: '" << standData["Code"] << "',\n";
                            if (standData.contains("Use"))
                                htmlFile << "            use: '" << standData["Use"] << "',\n";
                            if (standData.contains("Schengen"))
                                htmlFile << "            Schengen: " << (standData["Schengen"].get<bool>() ? "true" : "false") << ",\n";
                            if (standData.contains("Apron"))
                                htmlFile << "            Apron: " << (standData["Apron"].get<bool>() ? "true" : "false") << ",\n";
                            if (standData.contains("Priority"))
                                htmlFile << "            Priority: " << standData["Priority"] << ",\n";
                            htmlFile << "        };\n";

                            htmlFile << "        var circle_" << standNameVar << " = L.circle([" << lat << ", " << lon << "], {\n";
                            htmlFile << "            radius: " << radius << ",\n";
                            htmlFile << "            color: getStandColor(stand_" << standNameVar << "),\n";
                            htmlFile << "            fillColor: getStandColor(stand_" << standNameVar << "),\n";
                            htmlFile << "            fillOpacity: getStandOpacity(stand_" << standNameVar << ")\n";
                            htmlFile << "        }).addTo(map);\n";
                            htmlFile << "        currentStandElements.push(circle_" << standNameVar << ");\n";

                            // Create popup content
                            htmlFile << "        var popupContent_" << standNameVar << " = '<div class=\"stand-info\">Stand: " << standName << "</div>';\n";
                            if (standData.contains("Code"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Code: " << standData["Code"] << "';\n";
                            if (standData.contains("Use"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Use: " << standData["Use"] << "';\n";
                            if (standData.contains("Schengen"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Schengen: " << (standData["Schengen"].get<bool>() ? "Yes" : "No") << "';\n";
                            if (standData.contains("Apron"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Apron: " << (standData["Apron"].get<bool>() ? "Yes" : "No") << "';\n";
                            if (standData.contains("Priority"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Priority: " << standData["Priority"] << "';\n";
                            htmlFile << "        popupContent_" << standNameVar << " += '<br>Radius: " << radius << "m';\n";
                            htmlFile << "        popupContent_" << standNameVar << " += '<br>Coordinates: " << coords << "';\n";

                            // Add arrays if they exist
                            if (standData.contains("Callsigns") && standData["Callsigns"].is_array())
                            {
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Callsigns: ";
                                for (auto it = standData["Callsigns"].begin(); it != standData["Callsigns"].end(); ++it)
                                {
                                    if (it != standData["Callsigns"].begin())
                                        htmlFile << ", ";
                                    htmlFile << *it;
                                }
                                htmlFile << "';\n";
                            }

                            if (standData.contains("Countries") && standData["Countries"].is_array())
                            {
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Countries: ";
                                for (auto it = standData["Countries"].begin(); it != standData["Countries"].end(); ++it)
                                {
                                    if (it != standData["Countries"].begin())
                                        htmlFile << ", ";
                                    htmlFile << *it;
                                }
                                htmlFile << "';\n";
                            }

                            if (standData.contains("Block") && standData["Block"].is_array())
                            {
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Blocked: ";
                                for (auto it = standData["Block"].begin(); it != standData["Block"].end(); ++it)
                                {
                                    if (it != standData["Block"].begin())
                                        htmlFile << ", ";
                                    htmlFile << *it;
                                }
                                htmlFile << "';\n";
                            }

                            htmlFile << "        circle_" << standNameVar << ".bindPopup(popupContent_" << standNameVar << ");\n";

                            // Add click event to circle for coordinate copying
                            htmlFile << "        circle_" << standNameVar << ".on('click', function(e) {\n";
                            htmlFile << "            var lat = e.latlng.lat.toFixed(6);\n";
                            htmlFile << "            var lng = e.latlng.lng.toFixed(6);\n";
                            htmlFile << "            var coordString = lat + ':' + lng;\n";
                            htmlFile << "            \n";
                            htmlFile << "            // Copy to clipboard\n";
                            htmlFile << "            if (navigator.clipboard && window.isSecureContext) {\n";
                            htmlFile << "                navigator.clipboard.writeText(coordString).then(function() {\n";
                            htmlFile << "                    console.log('Coordinates copied: ' + coordString);\n";
                            htmlFile << "                }).catch(function(err) {\n";
                            htmlFile << "                    console.error('Failed to copy coordinates: ', err);\n";
                            htmlFile << "                });\n";
                            htmlFile << "            } else {\n";
                            htmlFile << "                // Fallback for older browsers\n";
                            htmlFile << "                var textArea = document.createElement('textarea');\n";
                            htmlFile << "                textArea.value = coordString;\n";
                            htmlFile << "                document.body.appendChild(textArea);\n";
                            htmlFile << "                textArea.select();\n";
                            htmlFile << "                document.execCommand('copy');\n";
                            htmlFile << "                document.body.removeChild(textArea);\n";
                            htmlFile << "            }\n";
                            htmlFile << "            // Don't prevent the popup from showing\n";
                            htmlFile << "        });\n\n";

                            // Calculate width based on stand name length
                            int labelWidth = std::max(30, static_cast<int>(standName.length()) * 8);

                            // Add stand label
                            htmlFile << "        var marker_" << standNameVar << " = L.marker([" << lat << ", " << lon << "], {\n";
                            htmlFile << "            icon: L.divIcon({\n";
                            htmlFile << "                className: 'stand-label',\n";
                            htmlFile << "                html: '<div style=\"background-color: rgba(255,255,255,0.8); padding: 2px 4px; border-radius: 3px; font-weight: bold; font-size: 12px; color: black; text-align: center; display: flex; align-items: center; justify-content: center; width: 100%; height: 100%; box-sizing: border-box;\">" << standName << "</div>',\n";
                            htmlFile << "                iconSize: [" << labelWidth << ", 20],\n";
                            htmlFile << "                iconAnchor: [" << labelWidth / 2 << ", 10]\n";
                            htmlFile << "            })\n";
                            htmlFile << "        }).addTo(map);\n";
                            htmlFile << "        currentStandElements.push(marker_" << standNameVar << ");\n\n";
                        }
                        catch (...)
                        {
                            std::cout << YELLOW << "Warning: Invalid coordinates for stand " << standName << RESET << std::endl;
                        }
                    }
                }
            }
        }

        // Emit JS to adjust view to include all stands (fitBounds) or center on single stand
        if (validStands == 0) {
            // keep default center/zoom
        } else if (validStands == 1) {
            htmlFile << "        // Center on single stand\n";
            htmlFile << "        map.setView([" << firstLat << ", " << firstLon << "], 16);\n";
        } else {
            htmlFile << "        // Fit map to bounds covering all stands\n";
            htmlFile << "        var bounds = L.latLngBounds([ [" << minLat << ", " << minLon << "], [" << maxLat << ", " << maxLon << "] ]);\n";
            htmlFile << "        map.fitBounds(bounds, { padding: [80, 80] });\n";
        }

        htmlFile << R"(        
        // Add legend
        var legend = L.control({position: 'topright'});
        legend.onAdd = function (map) {
            var div = L.DomUtil.create('div', 'legend');
            div.innerHTML = '<h4>)"
                 << icao << R"( Stands Legend</h4>' +
                '<i style="background:#96CEB4; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Default<br>' +
                '<i style="background:#45B7D1; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Schengen<br>' +
                '<i style="background:#4ECDC4; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Non-Schengen<br>' +
                '<i style="background:#FF6B6B; width:18px; height:18px; float:left; margin-right:8px; opacity:0.7; border-radius:50%;"></i> Apron<br><br>' +
                '<div style="background:#e8f5e8; padding:4px; border-radius:3px; margin:5px 0;"><strong>🔄 Auto-reload: ON</strong><br><small>Smart updates</small></div>' +
                '<small>Click circles for details<br>Click map to copy coordinates</small>';
            return div;
        };
        legend.addTo(map);
        
        // Add click event to copy coordinates to clipboard
        map.on('click', function(e) {
            var lat = e.latlng.lat.toFixed(6);
            var lng = e.latlng.lng.toFixed(6);
            var coordString = lat + ':' + lng;
            
            // Copy to clipboard
            if (navigator.clipboard && window.isSecureContext) {
                navigator.clipboard.writeText(coordString).then(function() {
                    // Show temporary popup at click location
                    var popup = L.popup()
                        .setLatLng(e.latlng)
                        .setContent('<div style="text-align: center;"><strong>Coordinates copied!</strong><br>' + coordString + '</div>')
                        .openOn(map);
                    
                    // Auto-close popup after 2 seconds
                    setTimeout(function() {
                        map.closePopup(popup);
                    }, 2000);
                }).catch(function(err) {
                    console.error('Failed to copy coordinates: ', err);
                    // Fallback method
                    fallbackCopyTextToClipboard(coordString, e.latlng);
                });
            } else {
                // Fallback for older browsers
                fallbackCopyTextToClipboard(coordString, e.latlng);
            }
        });
        
        // Fallback copy function for older browsers
        function fallbackCopyTextToClipboard(text, latlng) {
            var textArea = document.createElement("textarea");
            textArea.value = text;
            
            // Avoid scrolling to bottom
            textArea.style.top = "0";
            textArea.style.left = "0";
            textArea.style.position = "fixed";
            
            document.body.appendChild(textArea);
            textArea.focus();
            textArea.select();
            
            try {
                var successful = document.execCommand('copy');
                var popup = L.popup()
                    .setLatLng(latlng)
                    .setContent('<div style="text-align: center;"><strong>' + 
                               (successful ? 'Coordinates copied!' : 'Copy failed - please copy manually') + 
                               '</strong><br>' + text + '</div>')
                    .openOn(map);
                
                setTimeout(function() {
                    map.closePopup(popup);
                }, 2000);
            } catch (err) {
                console.error('Fallback: Oops, unable to copy', err);
            }
            
            document.body.removeChild(textArea);
        }
        
        // Add layer control
        var baseMaps = {
            "Satellite": L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}'),
            "OpenStreetMap": L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png')
        };
        L.control.layers(baseMaps).addTo(map);
        
        // Live reload system for localhost server
        if (window.location.protocol === 'http:' && window.location.hostname === 'localhost') {
            console.log('🔄 Live reload enabled - monitoring for changes');
            
            var lastReloadCheck = 0;
            var reloadCheckInterval;
            
            function checkForReload() {
                fetch('/reload_signal.txt?t=' + Date.now())
                    .then(response => response.text())
                    .then(timestamp => {
                        var currentCheck = parseInt(timestamp);
                        if (currentCheck > lastReloadCheck && lastReloadCheck > 0) {
                            console.log('✅ File updated! Reloading page...');
                            window.location.reload(true);
                        }
                        lastReloadCheck = currentCheck;
                    })
                    .catch(error => {
                        console.log('Reload check failed:', error.message);
                    });
            }
            
            // Check every 2 seconds
            reloadCheckInterval = setInterval(checkForReload, 2000);
            
            // Initialize check
            setTimeout(checkForReload, 1000);
            
            // Add visual indicator
            var indicator = document.createElement('div');
            indicator.innerHTML = '🔄 Live Reload Active';
            indicator.style.cssText = 'position:fixed;top:10px;right:10px;background:linear-gradient(45deg, #4CAF50, #45a049);color:white;padding:8px 12px;border-radius:8px;font-size:12px;z-index:10000;font-family:Arial,sans-serif;box-shadow:0 2px 10px rgba(0,0,0,0.3);border:1px solid rgba(255,255,255,0.2);';
            document.body.appendChild(indicator);
            
            // Animate indicator
            indicator.style.transform = 'translateY(-100px)';
            indicator.style.transition = 'all 0.3s ease';
            setTimeout(function() {
                indicator.style.transform = 'translateY(0)';
            }, 100);
            
            // Fade after 3 seconds
            setTimeout(function() {
                if (indicator && indicator.parentNode) {
                    indicator.style.opacity = '0.6';
                    indicator.innerHTML = '🔄 Monitoring...';
                }
            }, 3000);
        }
          
    </script>
</body>
<!-- Generated: )"
                 << timestamp << R"( -->
</html>)";

        htmlFile.close();

        std::cout << GREEN << "HTML map generated: " << filename << RESET << std::endl;

        if (!g_liveServer)
        {
            g_liveServer = std::make_unique<LiveReloadServer>();
            g_liveServer->startServer(filename);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }

        if (openBrowser)
        {
#ifdef _WIN32
            std::string mapFileName = std::filesystem::path(filename).filename().string();
            std::string localhost_url = "http://localhost:" + std::to_string(g_liveServer->getPort()) + "/" + mapFileName;
            std::string openCommand = "start \"\" \"" + localhost_url + "\"";
            std::system(openCommand.c_str());
            std::cout << "Map opened at " << localhost_url << std::endl;
#else
            std::cout << "Map generated at " << filename << std::endl;
#endif
        }
    }
}
