workspace "Shark"
	architecture "x64"
	startproject "SharkFin"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

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

group "Dependencies"
	include "Shark/dependencies/ImGui"
	include "Shark/dependencies/yaml-cpp"
	include "Shark/dependencies/box2d"
	include "Shark/dependencies/ImGuizmo"
	include "Shark/dependencies/fmt"
	include "Shark/dependencies/Optick"
group ""

include "Shark"
include "Sandbox"
include "SharkFin"