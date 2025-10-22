# Ramp Agent Config Creator
Helper app for creating NeoSTAND configs
<img width="929" height="321" alt="image" src="https://github.com/user-attachments/assets/c8f27f78-722d-48a9-8bb5-37515cab1742" />
<img width="826" height="728" alt="image" src="https://github.com/user-attachments/assets/0a8490a4-ea9a-4720-8333-123229f02620" />

## List of available commands
- `help` : display all available commands
- `add <standName>` : add new stand
- `remove <standName>` : remove existing stand
- `copy <sourceStand>` : copy existing stand settings
- `softcopy <sourceStand>` : copy existing stand settings but iterate through them so you can modify
- `edit <standName>` : edit existing stand
- `list` : list all stands
- `map` : generate HTML map visualization for debugging
- `save` : save changes and exit
- `exit` : exit without saving

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
- **Seamless updates** - Once a map is generated, it automatically updates when you:
  - Add new stands (`add`)
  - Edit existing stands (`edit`)
  - Remove stands (`remove`)
  - Copy stands (`copy` or `softcopy`)
  - Save changes (`save`)

### Usage:
1. Run the `map` command once to generate and open the map in your browser
2. Make any changes to your stands using other commands
3. The map will automatically update in the background
4. Simply refresh your browser tab to see the latest changes
5. When you `save`, the map is also updated to reflect the final state

The generated HTML file (`{ICAO}_map.html`) can be opened in any web browser and requires an internet connection to load the map tiles.
