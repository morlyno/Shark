# Shark
Shark is a 2D and 3D game engine written in C++, with support for scripting in C#.
The engine provides a simple editor to create your scenes and bring them to life using the physics system or C# scripts.

The project began as a way to learn more about rendering, physics and the details of how game engines work.

## Main features
The engine consists of a collection of features to drive the development of the engine itself and the creation of games.
Some of the most important features include:
- 2D/3D Renderer with DirectX11
- 2D Physics with [Box2D](https://github.com/erincatto/box2d.git)
- Scripting in C#
- Asset system
- Prefabs

#### Planned features
- 3D Physics with [JoltPhysics](https://github.com/jrouwe/JoltPhysics.git) (WIP on the Physics branch)
- Rendering features such as Shadows and Bloom

## Requirements
- Windows (only tested on Windows 10)
- [Visual Studio 2022](https://visualstudio.com) (not strictly required, but the included setup scripts only support it)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

## Getting Started
### <ins>1. Getting the source Code</ins>
Start by cloning the repository with `git clone --recursive https://github.com/morlyno/Shark`.

If the repository was not cloned recursively, the setup script will clone and initialize the required submodules.

### <ins>1. Generating the Solution</ins>
To generate the Visual Studio 2022 solution file navigate to the `Scripts` folder and run [Setup.bat](https://github.com/morlyno/Shark/blob/main/Scripts/Setup.bat). 

If you simply want to regenerate the solution, run the [Win-GenerateProjects.bat](https://github.com/morlyno/Shark/blob/main/Scripts/Win-GenerateProjects.bat) script located in the `Scripts` folder.
