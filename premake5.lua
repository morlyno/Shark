workspace "Shark"
	architecture "x64"
	startproject "SharkFin"

	configurations
	{
		"Debug",
		"Release"
	}

    flags
    {
        "MultiProcessorCompile"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

MonoDir = os.getenv("MONO_PROJECT")

IncludeDir = {}
IncludeDir["spdlog"] = "%{wks.location}/Shark/dependencies/spdlog/include"
IncludeDir["ImGui"] = "%{wks.location}/Shark/dependencies/ImGui"
IncludeDir["stb_image"] = "%{wks.location}/Shark/dependencies/stb_image"
IncludeDir["EnTT"] = "%{wks.location}/Shark/dependencies/EnTT/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Shark/dependencies/yaml-cpp/include"
IncludeDir["box2d"] = "%{wks.location}/Shark/dependencies/box2d/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Shark/dependencies/ImGuizmo"
IncludeDir["fmt"] = "%{wks.location}/Shark/dependencies/fmt/include"
IncludeDir["Optick"] = "%{wks.location}/Shark/dependencies/Optick/src"
IncludeDir["glm"] = "%{wks.location}/Shark/dependencies/glm"
IncludeDir["Mono"] = "%{MonoDir}/include/mono-2.0"

Library = {}
Library["Mono_lib"] = "%{MonoDir}/lib/mono-2.0-sgen.lib"
Library["Mono_dll"] = "%{MonoDir}/bin/mono-2.0-sgen.dll"

Symbols = {}
Symbols["Mono_lib"] = "%{MonoDir}/lib/mono-2.0-sgen.pdb"
Symbols["Mono_dll"] = "%{MonoDir}/bin/mono-2.0-sgen.pdb"

group "Dependencies"
	include "Shark/dependencies/ImGui"
	include "Shark/dependencies/yaml-cpp"
	include "Shark/dependencies/box2d"
	include "Shark/dependencies/ImGuizmo"
	include "Shark/dependencies/Optick"
group ""

include "Shark"
include "Sandbox"
include "SharkFin"
include "ScriptingCore"