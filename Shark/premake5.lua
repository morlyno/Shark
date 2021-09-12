project "Shark"
    kind "StaticLib"
    language "c++"
    cppdialect "c++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skpch.h"
    pchsource "src/skpch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
        "%{includeDir.stb_image}/*.h",
        "%{includeDir.stb_image}/*.cpp"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{includeDir.spdlog}",
        "%{includeDir.ImGui}",
        "%{includeDir.stb_image}",
        "%{includeDir.EnTT}",
        "%{includeDir.yaml_cpp}",
        "%{includeDir.box2d}",
        "%{includeDir.ImGuizmo}",
        "%{includeDir.fmt}"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

        links
        {
            "d3d11",
            "dxgi",
            "ImGui",
            "yaml-cpp",
            "box2d",
            "ImGuizmo",
            "fmt"
        }

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
