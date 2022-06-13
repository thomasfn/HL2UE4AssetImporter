# HL2UE4 Asset Importer

A WIP Unreal Engine 4 plugin that adds support for importing various Source engine file format, with a long-term goal of being able to import the entirety of Half-Life 2 into the UE4 editor and play through the full game.

## Project Status

Please see [Project Status](STATUS.md) for a more detailed list.

- Key
	- :heavy_check_mark: Full Support
	- :heavy_exclamation_mark: Partial Support / WIP
	- :x: No Support

- :heavy_exclamation_mark: Textures (.vtf) v<=7.5.3
- :heavy_exclamation_mark: Materials (.vmt)
- :heavy_check_mark: Static Models (.mdl) v44-48
- :heavy_exclamation_mark: Animated Models (.mdl) v44-48
- :heavy_exclamation_mark: Maps (.bsp) v19-20
- :x: Sounds
- :x: Effects and Particles

### Entities and Logic
	
- :heavy_exclamation_mark: Entities (from .bsp)
- :heavy_exclamation_mark: Player
- :x: NPCs

### Other

- :heavy_exclamation_mark: Lighting
- :heavy_exclamation_mark: Physics

## Getting Started

This project is architected as a UE4 plugin, comprising of an editor module and a runtime module.

### Prerequisites

- Unreal Engine 4.25
- Windows 64/32 bit (Linux/Mac not yet supported)
- [Half-Life 2](https://store.steampowered.com/app/220/HalfLife_2/) or [Half-Life 2 Update](https://store.steampowered.com/app/290930/HalfLife_2_Update/)

### Installing

1. Create a new UE4 project - choose a blank empty project with no default assets and no default code. Close the editor.
2. Create a folder in the project folder (next to the .uproject) called "Plugins".
3. Create a folder inside "Plugins" called "HL2AssetImporter" - clone the repo into there.
4. Load up the UE4 project. It should talk about compiling the plugin for the first time.
5. Go to Edit -> Plugins, go to the "Project" category and find "HL2.AssetImporter".
6. If "Enabled" is not ticked, tick it and restart the editor.
7. If all has gone well, you should see a new "HL2" icon in the main tool bar.

 ![Screenshot of toolbar](https://github.com/thomasfn/HL2UE4AssetImporter/raw/master/docs/toolbar1.png "Screenshot of toolbar")

### Importing HL2

Note that the following steps, especially the bulk imports, will take a long time - depending on if your project and extracted HL2 assets are on SSDs or not, and the strength of your CPU. Expect to put aside an hour or more for the whole process.

There may also be certain assets that fail to import. This won't cause issues usually, but you do need to watch it and click ok otherwise it won't continue with the bulk import.

After every step you should save (File -> Save All) otherwise, if it crashes, you'll have to start all over again.

1. Use [GCFScape](http://nemesis.thewavelength.net/?p=26) or another tool to extract the HL2 vpks to somewhere on disc.
2. From the "HL2" toolbar icon, click "Bulk Import Textures". Navigate to the extracted HL2 "materials" folder.
3. From the "HL2" toolbar icon, click "Convert Skyboxes".
4. From the "HL2" toolbar icon, click "Bulk Import Materials". Navigate to the extracted HL2 "materials" folder.
5. From the "HL2" toolbar icon, click "Bulk Import Models". Navigate to the extracted HL2 "models" folder.
6. Create a new empty level.
7. From the "HL2" toolbar icon, click "Import BSP". Navigate to the extracted HL2 "maps" folder. Select a map like d1_trainstation_01.
8. Click "Build" to build static lighting (or don't and run with dynamic lighting for now).
9. TODO: Instructions on how to setup inputs for player interaction.

## Running the tests

There are only a couple of unit tests for some core systems so there's not much point in running these religiously (yet).

1. Go to Edit -> Plugins, find and enable the "Editor Tests", "Runtime Tests" and "Functional Testing Editor" plugins.
2. Restart the editor.
3. Go to Window -> Test Automation.
4. Tick the category "HL2" and click "Start Tests".

## Contributing

Please read [CONTRIBUTING.md](https://gist.github.com/PurpleBooth/b24679402957c63ec426) for details on our code of conduct, and the process for submitting pull requests to us.

## Authors

* **[thomasfn](https://github.com/thomasfn)** - *Initial work*

See also the list of [contributors](https://github.com/your/project/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.

Note that there is some third party code included with the project that is NOT licensed under MIT, this code is kept in separate folders with their own licenses.

## Acknowledgments

* [VTFLib](http://nemesis.thewavelength.net/index.php?c=177)
* [ValveBSP](https://github.com/ReactiioN1337/valve-bsp-parser/tree/master/ValveBSP)
* [Valve Developer Community](https://developer.valvesoftware.com/wiki/Main_Page)
