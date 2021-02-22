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

group "Dependencies"
	include "dependencies/Premake"
	include "Shark/dependencies/ImGui"
group ""

include "Shark"
include "Sandbox"
include "SharkFin"