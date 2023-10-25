workspace "Shark"
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

	filter "language:C++ or Language:C"
		architecture "x86_64"
	filter ""

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "Dependencies.lua"

DefaultDefines = {
	"_CRT_SECURE_NO_WARNINGS",
	"_USE_MATH_DEFINES",

	ImGui.Defines,
	glm.Defines,
	fmt.Defines,
	yaml_cpp.Defines,
	tracy.Defines
}

group "Dependencies"
	include "Shark/dependencies/Box2D"
	include "Shark/dependencies/ImGui"
	include "Shark/dependencies/ImGuizmo"
	include "Shark/dependencies/msdf-atlas-gen"
	include "Shark/dependencies/yaml-cpp"
group ""

include "Shark"
include "SharkFin"
include "Shark-Runtime"
include "Shark-ScriptCore"