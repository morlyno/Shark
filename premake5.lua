workspace "Shark"
	architecture "x64"
	startproject "SharkFin"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

MonoDir = os.getenv("MONO_PROJECT")

includeDir = {}
includeDir["spdlog"] = "%{wks.location}/Shark/dependencies/spdlog/include"
includeDir["ImGui"] = "%{wks.location}/Shark/dependencies/ImGui"
includeDir["stb_image"] = "%{wks.location}/Shark/dependencies/stb_image"
includeDir["EnTT"] = "%{wks.location}/Shark/dependencies/EnTT/include"
includeDir["yaml_cpp"] = "%{wks.location}/Shark/dependencies/yaml-cpp/include"
includeDir["box2d"] = "%{wks.location}/Shark/dependencies/box2d/include"
includeDir["ImGuizmo"] = "%{wks.location}/Shark/dependencies/ImGuizmo"
includeDir["fmt"] = "%{wks.location}/Shark/dependencies/fmt/include"
includeDir["Optick"] = "%{wks.location}/Shark/dependencies/Optick/src"
includeDir["glm"] = "%{wks.location}/Shark/dependencies/glm"
includeDir["Mono"] = "%{MonoDir}/include/mono-2.0"

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