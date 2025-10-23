# Ramp Agent Config Creator
Helper CLI to create Ramp Agent config files
<img width="924" height="683" alt="image" src="https://github.com/user-attachments/assets/bfc6feb9-e2ab-40f4-84f8-999b3268d156" />

<img width="826" height="728" alt="image" src="https://github.com/user-attachments/assets/0a8490a4-ea9a-4720-8333-123229f02620" />

## List of available commands
- `help` : display all available commands
- `add <standName>` : add new stand
- `remove <standName>` : remove existing stand
- `copy <sourceStand>` : copy existing stand settings
- `batchcopy <sourceStand>` : copy existing stand settings to a list of stand + coordinates
- `softcopy <sourceStand>` : copy existing stand settings but iterate through them so you can modify
- `edit <standName>` : edit existing stand
- `list` : list all stands
- !`map` : generate HTML map visualization for debugging
- `save` : save changes and exit
- `exit` : exit without saving

**!** Python required for the map visualisation

## Debug Map Visualization

The `map` command generates an interactive HTML map that visualizes all stands with their radii and properties. This is perfect for debugging and verifying stand positions.

### Features:
- **Satellite imagery** from Esri with OpenStreetMap overlay for airport details
- **Color-coded stands** based on type:
  - 🟢 Default stands (green)
  - 🔵 Schengen stands (blue) 
  - 🔵 Non-Schengen stands (teal)
  - 🔴 Apron stands (red)
- **Interactive circles** showing each stand's radius
- **Detailed popups** with all stand information (code, use, priority, callsigns, countries, blocked stands, etc.)
- **Stand labels** for easy identification
- **Legend** and layer controls
- **🔄 Live Reload Server** - Automatic browser refresh when files change:
  - Starts a local server at `http://localhost:3000`
  - Real-time updates without manual refresh
  - Works when you add, edit, remove, or copy stands
  - Smart file monitoring with WebSocket-like updates

### Usage:
1. Run the `map` command once to generate the map and start the live reload server
2. The map opens automatically at `http://localhost:4000/[ICAO]_map.html`
3. Make any changes to your stands using other commands
4. **The map automatically refreshes in your browser** when changes are detected
5. No need to manually refresh - changes appear instantly!

The generated HTML file (`{ICAO}_map.html`) can be opened in any web browser and requires an internet connection to load the map tiles.

### Live Reload Requirements:
- **Python 3.x** must be installed and available in PATH
- The live reload server automatically starts when you use the `map` command
- Server runs on `localhost:4000` (automatically finds available port)
- The server stops when you `exit` the application
