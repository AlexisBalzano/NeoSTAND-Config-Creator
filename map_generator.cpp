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
                        if (lat < minLat)
                            minLat = lat;
                        if (lat > maxLat)
                            maxLat = lat;
                        if (lon < minLon)
                            minLon = lon;
                        if (lon > maxLon)
                            maxLon = lon;
                        if (validStands == 0)
                        {
                            firstLat = lat;
                            firstLon = lon;
                        }
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
        
        // Store references to current stands for cleanup
        var currentStandElements = [];
        
        // Color function for different stand types
        function getStandColor(standData) {
            return '#96CEB4';  // Green for default
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
                                htmlFile << "            Code: '" << standData["Code"] << "',\n";
                            if (standData.contains("Use"))
                                htmlFile << "            Use: '" << standData["Use"] << "',\n";
                            if (standData.contains("Schengen"))
                                htmlFile << "            Schengen: " << (standData["Schengen"].get<bool>() ? "true" : "false") << ",\n";
                            if (standData.contains("Apron"))
                                htmlFile << "            Apron: " << (standData["Apron"].get<bool>() ? "true" : "false") << ",\n";
                            if (standData.contains("Remark"))
                                htmlFile << "            Remark: '" << standData["Remark"] << "',\n";
                            if (standData.contains("Wingspan"))
                                htmlFile << "            Wingspan: '" << standData["Wingspan"] << "',\n";
                            if (standData.contains("Callsigns"))
                                htmlFile << "            Callsigns: '" << standData["Callsigns"] << "',\n";
                            if (standData.contains("Priority"))
                                htmlFile << "            Priority: " << standData["Priority"] << ",\n";
                            htmlFile << "        };\n";

                            htmlFile << "        var circle_" << standNameVar << " = L.circle([" << lat << ", " << lon << "], {\n";
                            htmlFile << "            radius: " << radius << ",\n";
                            htmlFile << "            color: getStandColor(stand_" << standNameVar << "),\n";
                            htmlFile << "            fillColor: getStandColor(stand_" << standNameVar << "),\n";
                            htmlFile << "            fillOpacity: 0.4\n";
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
                            if (standData.contains("Wingspan"))
                                htmlFile << "        popupContent_" << standNameVar << " += '<br>Wingspan: " << standData["Wingspan"].get<double>() << "m';\n";
                            if (standData.contains("Remark")) {
                                for (const auto& [code, remark] : standData["Remark"].items())
                                    htmlFile << "        popupContent_" << standNameVar << " += '<br>Remark (" << code << "): " << remark << "';\n";
                            }
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
        if (validStands == 0)
        {
            // keep default center/zoom
        }
        else if (validStands == 1)
        {
            htmlFile << "        // Center on single stand\n";
            htmlFile << "        map.setView([" << firstLat << ", " << firstLon << "], 16);\n";
        }
        else
        {
            htmlFile << "        // Fit map to bounds covering all stands\n";
            htmlFile << "        var bounds = L.latLngBounds([ [" << minLat << ", " << minLon << "], [" << maxLat << ", " << maxLon << "] ]);\n";
            htmlFile << "        map.fitBounds(bounds, { padding: [80, 80] });\n";
        }

        htmlFile << R"(
        
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

        /* Color-mode control UI */
        (function() {
            // Create control container
            var controlBtn = document.createElement('div');
            controlBtn.style.cssText = 'position:fixed;right:12px;top:50%;transform:translateY(-50%);z-index:10001;';
            document.body.appendChild(controlBtn);

            // Button
            var toggleBtn = document.createElement('button');
            toggleBtn.innerHTML = '🎨 Colors';
            toggleBtn.title = 'Change stand coloring';
            toggleBtn.style.cssText = 'background:#ffffffaa;border:1px solid rgba(0,0,0,0.15);padding:8px 10px;border-radius:6px;cursor:pointer;font-weight:600;box-shadow:0 2px 8px rgba(0,0,0,0.15);';
            controlBtn.appendChild(toggleBtn);

            // Sliding panel
            var panel = document.createElement('div');
            panel.style.cssText = 'position:fixed;right:12px;top:50%;transform:translateY(-50%) translateX(110%);width:260px;z-index:10002;background:#fff;border:1px solid rgba(0,0,0,0.12);box-shadow:0 6px 20px rgba(0,0,0,0.12);padding:12px;border-radius:8px;transition:transform 0.25s ease;max-height:70vh;overflow:auto;font-family:Arial,sans-serif;';
            panel.innerHTML = '<strong>Color stands by</strong><br/><small style="color:#666">Choose a mode to recolor visible stands</small><hr style="margin:8px 0"/>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="default" checked/> Default </label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="schengen"/> Schengen / Non-Schengen</label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="apron" /> Apron / Stand</label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="use" /> Use </label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="priority" /> Priority (gradient)</label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="codeHighest" /> Code (highest)</label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="codeAll" /> Code (All)</label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="remark" /> Remark / no Remark </label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="wingspan" /> Wingspan / no Wingspan </label>' +
                  '<label style="display:block;margin:6px 0"><input type="radio" name="colorMode" value="callsign" /> Callsign / no Callsign </label>' +
                  '<hr style="margin:8px 0"/>' +
                  '<div id="colorModeInfo" style="font-size:12px;color:#444">Current: Default</div>';
            document.body.appendChild(panel);

            var open = false;
            toggleBtn.addEventListener('click', function() {
            open = !open;
            panel.style.transform = open ? 'translateY(-50%) translateX(0)' : 'translateY(-50%) translateX(110%)';
            });

            // Close panel when clicking outside
            document.addEventListener('click', function(e) {
            if (!panel.contains(e.target) && !toggleBtn.contains(e.target) && open) {
            open = false;
            panel.style.transform = 'translateY(-50%) translateX(110%)';
            }
            });

            // Utility: HSL gradient for priority (-100..100) => blue (240) -> green (120) -> red (0)
            function colorForPriority(p) {
            if (p === undefined || p === null || isNaN(p)) return '#96CEB4';
            var v = Number(p);
            // Clamp to -100..100
            v = Math.max(-100, Math.min(100, v));
            // Normalize to 0..1 where -100 => 0, 0 => 0.5, 100 => 1
            var t = (v + 100) / 200;
            // Map to hue: 240 (blue) -> 120 (green) -> 0 (red)
            var hue = 240 * (1 - t);
            return 'hsl(' + Math.round(hue) + ',70%,50%)';
            }

            // Build a set of stands by inspecting global vars like stand_<name>
            function collectStands() {
            var list = [];
            for (var k in window) {
            if (!Object.prototype.hasOwnProperty.call(window, k)) continue;
            if (k.indexOf('stand_') === 0) {
            try {
                var stand = window[k];
                var shortName = k.substring(6); // suffix used for circle_ and marker_
                var circle = window['circle_' + shortName];
                var marker = window['marker_' + shortName];
                // Only include if a circle exists
                if (circle) {
                list.push({ id: shortName, stand: stand, circle: circle, marker: marker });
                }
            } catch (e) {
                // ignore
            }
            }
            }
            return list;
            }

            // Determine color by selected mode
            function determineColor(mode, stand) {
            if (mode === 'default') {
            return '#96CEB4';
            }
            if (mode === 'schengen') {
            if (stand.Schengen === false) return '#4e70cdff';
            if (stand.Schengen === true) return '#45B7D1';
            return '#96CEB4';
            }
            if (mode === 'apron') {
            return stand.Apron ? '#FF6B6B' : '#96CEB4';
            }
            if (mode === 'use') {
            // categorical mapping for common uses. New categories get assigned deterministic colors.
            var map = {
            'Commercial': '#4E79A7',
            'Cargo': '#59A14F',
            'Military': '#E15759',
            'General': '#F28E2B',
            'Maintenance': '#B07AA1',
            'Default': '#96CEB4'
            };
            var u = stand.Use || stand.use || 'Default';
            if (map[u]) return map[u];
            // deterministic color by hashing the use string
            var hash = 0;
            for (var i = 0; i < u.length; i++) hash = (hash << 5) - hash + u.charCodeAt(i);
            var hue = Math.abs(hash) % 360;
            return 'hsl(' + hue + ',65%,55%)';
            }
            if (mode === 'priority') {
            var p = stand.Priority;
            return colorForPriority(p);
            }
            if (mode === 'codeHighest') {
            if (!stand.Code) return '#96CEB4';
            var code = stand.Code.toString().toUpperCase();
            var highestChar = 0;
            for (var i = 0; i < code.length; i++) {
                var c = code.charCodeAt(i);
                if (c > highestChar) highestChar = c;
            }
            var hue = (highestChar * 37) % 360; // arbitrary multiplier for distribution
            return 'hsl(' + hue + ',65%,55%)';
            }
            if (mode === 'codeAll') {
            if (!stand.Code) return '#96CEB4';
            var code = stand.Code.toString().toUpperCase();
            var hash = 0;
            for (var i = 0; i < code.length; i++) hash = (hash << 5) - hash + code.charCodeAt(i);
            var hue = Math.abs(hash) % 360;
            return 'hsl(' + hue + ',65%,55%)';
            }
            if (mode === 'remark') {
            return stand.Remark ? '#FFB347' : '#96CEB4';
            }
            if (mode === 'wingspan') {
            return stand.Wingspan ? '#FFB347' : '#96CEB4';
            }
            if (mode === 'callsign') {
            return (stand.Callsigns && stand.Callsigns.length > 0) ? '#FFB347' : '#96CEB4';
            }
            return '#96CEB4';
            }

            // Create legend control (top-right)
            var legend = L.control({ position: 'topright' });
            var legendDiv = null;
            legend.onAdd = function(map) {
            legendDiv = L.DomUtil.create('div', 'info legend');
            legendDiv.style.cssText = 'background:white;padding:8px;border-radius:6px;box-shadow:0 2px 6px rgba(0,0,0,0.15);font-size:12px;max-width:260px;max-height:40vh;overflow:auto;';
            legendDiv.innerHTML = '<strong style="display:block;margin-bottom:6px;">Legend</strong><div id="legendContent" style="line-height:1.3"></div>';
            return legendDiv;
            };
            legend.addTo(map);

            // Build HTML for a swatch+label
            function swatchHTML(color, label) {
            return '<div style="display:flex;align-items:center;margin:4px 0;"><span style="width:18px;height:14px;background:' + color + ';border:1px solid rgba(0,0,0,0.15);margin-right:8px;display:inline-block;border-radius:2px;"></span><span style="flex:1;word-break:break-word;">' + label + '</span></div>';
            }

            // Update legend based on current mode and available stands
            function updateLegend(mode, items) {
            var content = document.getElementById('legendContent');
            if (!content) return;
            content.innerHTML = ''; // clear

            if (!items || items.length === 0) {
            content.innerHTML = '<div style="color:#666">No stands found</div>';
            return;
            }

            // Helper: collect unique keys/values
            function unique(arr) {
            return Array.from(new Set(arr));
            }

            if (mode === 'default') {
            content.innerHTML = swatchHTML(determineColor('default', {}), 'Stands');
            return;
            }

            if (mode === 'schengen') {
            content.innerHTML += swatchHTML('#45B7D1', 'Schengen');
            content.innerHTML += swatchHTML('#4e70cdff', 'Non-Schengen');
            content.innerHTML += swatchHTML('#96CEB4', 'Either');
            return;
            }

            if (mode === 'apron') {
            content.innerHTML += swatchHTML('#FF6B6B', 'Apron');
            content.innerHTML += swatchHTML('#96CEB4', 'Stand');
            return;
            }

            if (mode === 'remark' || mode === 'wingspan' || mode === 'callsign') {
            var yesColor = '#FFB347';
            var noColor = '#96CEB4';
            var yesLabel = (mode === 'remark') ? 'Has Remark' : (mode === 'wingspan' ? 'Has Wingspan' : 'Has Callsign(s)');
            content.innerHTML += swatchHTML(yesColor, yesLabel);
            content.innerHTML += swatchHTML(noColor, 'None');
            return;
            }

            if (mode === 'priority') {
            // find min/max priority values present
            var vals = items.map(function(it) { return (it.stand && it.stand.Priority != null) ? Number(it.stand.Priority) : NaN; }).filter(function(v){ return !isNaN(v); });
            var min = vals.length ? Math.min.apply(null, vals) : -100;
            var max = vals.length ? Math.max.apply(null, vals) : 100;
            var grad = 'linear-gradient(90deg, ' + colorForPriority(min) + ' 0%,' + colorForPriority((min+max)/2) + ' 50%,' + colorForPriority(max) + ' 100%)';
            content.innerHTML += '<div style="display:flex;flex-direction:column;"><div style="height:14px;border-radius:4px;border:1px solid rgba(0,0,0,0.06);background:' + grad + ';margin-bottom:6px;"></div><div style="font-size:11px;color:#333;">Priority range: ' + min + ' — ' + max + ' (-100..100)</div></div>';
            return;
            }

            if (mode === 'use') {
            // collect distinct uses
            var uses = items.map(function(it){ return (it.stand && (it.stand.Use || it.stand.use)) ? (it.stand.Use || it.stand.use) : 'N/A'; });
            uses = unique(uses);
            uses.forEach(function(u){
            var color = determineColor('use', { Use: u });
            content.innerHTML += swatchHTML(color, u);
            });
            return;
            }
            
            if (mode === 'codeHighest') {
            // collect distinct highest chars
            var codes = items.map(function(it){
            if (it.stand && it.stand.Code) {
                var code = it.stand.Code.toString().toUpperCase();
                var highestChar = 0;
                for (var i = 0; i < code.length; i++) {
                var c = code.charCodeAt(i);
                if (c > highestChar) highestChar = c;
                }
                return String.fromCharCode(highestChar);
            }
            return null;
            }).filter(function(c){ return c !== null; });
            codes = unique(codes);
            codes.forEach(function(code){
            var color = determineColor('codeHighest', { Code: code });
            content.innerHTML += swatchHTML(color, 'Highest char: ' + code);
            });
            return;
            }

            if (mode === 'codeAll') {
            // collect distinct codes
            var codes = items.map(function(it){ return (it.stand && it.stand.Code) ? it.stand.Code.toString() : null; }).filter(function(c){ return c !== null; });
            codes = unique(codes);
            if (codes.length === 0) {
            content.innerHTML = '<div style="color:#666">No Code values found</div>';
            return;
            }
            // limit to reasonable number to avoid huge legend
            var maxEntries = 100;
            codes.slice(0, maxEntries).forEach(function(code){
            var color = determineColor(mode, { Code: code });
            content.innerHTML += swatchHTML(color, 'Code: ' + code);
            });
            if (codes.length > maxEntries) {
            content.innerHTML += '<div style="color:#666;font-size:11px;margin-top:6px;">... ' + (codes.length - maxEntries) + ' more entries omitted</div>';
            }
            return;
            }

            // fallback: try to show a few items with computed colors
            items.slice(0, 25).forEach(function(it){
            var label = (it.stand && it.stand.name) ? it.stand.name : it.id;
            var color = determineColor(mode, it.stand || {});
            content.innerHTML += swatchHTML(color, label);
            });
            }

            // Apply coloring to map elements
            function applyColoring(mode) {
            var items = collectStands();
            items.forEach(function(it) {
            try {
            var c = determineColor(mode, it.stand);
            // Set style on circle
            if (it.circle && typeof it.circle.setStyle === 'function') {
                it.circle.setStyle({ color: c, fillColor: c, fillOpacity: 0.45 });
            }
            // Optionally adjust marker background (small square around label)
            if (it.marker && it.marker._icon) {
                // try to find the inner div and adjust its background-color
                var inner = it.marker._icon.querySelector('div');
                if (inner) inner.style.backgroundColor = c;
            }
            } catch (e) {
            console.error('Failed coloring stand', it.id, e);
            }
            });

            // Update info text
            var info = document.getElementById('colorModeInfo');
            if (info) info.innerText = 'Current: ' + (mode.charAt(0).toUpperCase() + mode.slice(1));

            // Update legend to reflect the chosen mode and discovered properties
            try {
            updateLegend(mode, items);
            } catch (e) {
            console.error('Failed to update legend', e);
            }
            }

            // Wire radio buttons
            var radios = panel.querySelectorAll('input[name="colorMode"]');
            radios.forEach(function(r) {
            r.addEventListener('change', function(e) {
            applyColoring(e.target.value);
            });
            });

            // Initial apply (default)
            setTimeout(function() { applyColoring('default'); }, 50);

            // Expose quick API for console debugging
            window.__mapColoring = {
            apply: applyColoring,
            collect: collectStands,
            colorForPriority: colorForPriority
            };
        })();

        // Add legend
        //TODO: make legend dynamic based on current color configuration
        var legend = L.control({position: 'topright'});
        

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
            indicator.style.cssText = 'position:fixed;bottom:10px;right:10px;background:linear-gradient(45deg, #4CAF50, #45a049);color:white;padding:8px 12px;border-radius:8px;font-size:12px;z-index:10000;font-family:Arial,sans-serif;box-shadow:0 2px 10px rgba(0,0,0,0.3);border:1px solid rgba(255,255,255,0.2);';
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
