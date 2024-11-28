
include "Dependencies.lua"

workspace "Shark"
	configurations { "Debug", "Debug-AS", "Release" }
	startproject "Shark-Editor"
	conformancemode "On"

	language "C++"
	cppdialect "C++20"
	staticruntime "Off"

	flags { "MultiProcessorCompile" }

	defines {
		"_CRT_SECURE_NO_WARNINGS",
		"NOMINMAX",

		"TRACY_ENABLE",
		"TRACY_ON_DEMAND",
		"TRACY_CALLSTACK=10",

		"YAML_CPP_STATIC_DEFINE",

		"FMT_HEADER_ONLY",
		"FMT_UNICODE=0"
	}

	filter "action:vs*"
		linkoptions { "/ignore:4099" } -- ignore no PDB found warning
	
	filter "language:C++ or Language:C"
		architecture "x86_64"
		
	filter "configurations:Debug or configurations:Debug-AS"
		optimize "Off"
		symbols "On"
        defines { "SK_DEBUG", "_DEBUG" }

	filter "configurations:Debug-AS"
		sanitize { "Address" }
		flags { "NoRuntimeChecks", "NoIncrementalLink" }
		editandcontinue "off"
	
	filter "configurations:Release"
		optimize "On"
		symbols "Default"
        vectorextensions "AVX2"
        defines { "SK_RELEASE", "NDEBUG" }

	filter "system:windows"
		buildoptions { "/Zc:preprocessor", "/Zc:__cplusplus" }
        systemversion "latest"
        defines { "SK_PLATFORM_WINDOWS" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}"

group "Dependencies"
	include "Shark/dependencies/Box2D"
	include "Shark/dependencies/ImGui"
	include "Shark/dependencies/msdf-atlas-gen"
	include "Shark/dependencies/yaml-cpp"
	include "Shark/dependencies/Coral/Coral.Native"
	include "Shark/dependencies/Coral/Coral.Managed"
group ""

group "Core"
	include "Shark"
	include "Shark-ScriptCore"
group ""

group "Runtime"
	include "Shark-Runtime"
group ""

group "Tools"
	include "Shark-Editor"
group ""