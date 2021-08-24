project "SharkFin"
    kind "ConsoleApp"
    language "c++"
    cppdialect "c++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skfpch.h"
    pchsource "src/skfpch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
        "assets/NativeScripts/*.cpp"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{wks.location}/SharkFin/src",
        "%{includeDir.spdlog}",
        "%{includeDir.ImGui}",
        "%{includeDir.EnTT}",
        "%{includeDir.yaml_cpp}",
        "%{includeDir.box2d}",
        "%{includeDir.ImGuizmo}"
    }

    links
    {
        "Shark"
    }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
