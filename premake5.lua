workspace "Shark"
	architecture "x64"
	startproject "Sandbox"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

includeDir = {}
includeDir["spdlog"] = "Shark/dependencies/spdlog/include"
includeDir["ImGui"] = "Shark/dependencies/ImGui"

include "Shark/dependencies"

project "Shark"
	location "Shark"
	kind "StaticLib"
	language "c++"
	cppdialect "c++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "skpch.h"
	pchsource "Shark/src/skpch.cpp"

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}

	includedirs
	{
		"Shark/src",
		"%{includeDir.spdlog}",
		"%{includeDir.ImGui}"

	}

	filter "system:windows"
		systemversion "latest"

		defines
		{
			"SK_BUILD_DLL"
		}

		links
		{
			"d3d11.lib",
			"ImGui"
		}

	filter "configurations:Debug"
		defines "SK_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "SK_RELEASE"
		runtime "Release"
		optimize "on"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "c++"
	cppdialect "c++17"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
	
	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
	}
	
	includedirs
	{
		"Shark/src",
		"%{includeDir.spdlog}",
		"%{includeDir.ImGui}"
	}

	links
	{
		"Shark"
	}
	
	filter "system:windows"
		systemversion "latest"

		defines {}

	filter "configurations:Debug"
		defines "SK_DEBUG"
		runtime "Debug"
		symbols "on"
		
	filter "configurations:Release"
		defines "SK_RELEASE"
		runtime "Release"
		optimize "on"
