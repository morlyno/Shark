project "Shark"
    kind "StaticLib"
    language "c++"
    cppdialect "c++17"
    staticruntime "on"

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
        "%{includeDir.EnTT}"
    }

    filter "system:windows"
        systemversion "latest"

        links
        {
            "d3d11",
            "dxgi",
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
