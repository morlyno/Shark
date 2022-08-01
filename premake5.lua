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

Settings = {
	UseVectorExtensions = false
}

DefaultDefines = {
	"_USE_MATH_DEFINES",

	"IMGUI_DEFINE_MATH_OPERATORS",
	
	"GLM_FORCE_SWIZZLE",
	"GLM_FORCE_LEFT_HANDED",
	"GLM_FORCE_DEPTH_ZERO_TO_ONE",
	"GLM_FORCE_INTRINSICS",

	"FMT_HEADER_ONLY",
	"YAML_CPP_STATIC_DEFINE"
}

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
include "Shark-ScriptCore"