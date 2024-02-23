project "SharkFin"
    kind "ConsoleApp"
    language "c++"
    cppdialect "c++20"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skfpch.h"
    pchsource "src/skfpch.cpp"

    vectorextensions "AVX2"

    files
    {
        "src/**.h",
        "src/**.cpp",

        "Resources/Shaders/**.hlsl",
        "Resources/Shaders/**.glsl",
        "Resources/Shaders/**.glslh"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{wks.location}/SharkFin/src",
        "%{spdlog.IncludeDir}",
        "%{ImGui.IncludeDir}",
        "%{EnTT.IncludeDir}",
        "%{yaml_cpp.IncludeDir}",
        "%{Box2D.IncludeDir}",
        "%{ImGuizmo.IncludeDir}",
        "%{fmt.IncludeDir}",
        "%{glm.IncludeDir}",
        "%{tracy.IncludeDir}",
        "%{filewatch.IncludeDir}"
    }

    links
    {
        "Shark"
    }

    defines
    {
        DefaultDefines
    }

    postbuildcommands
    {
        '{COPYFILE} "%{Assimp.Binary}" "%{cfg.targetdir}"'
    }

    filter "files:**.hlsl or files:**.glsl or files:**.glslh"
        flags { "ExcludeFromBuild" }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "On"
        symbols "Default"

