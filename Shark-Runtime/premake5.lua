project "Shark-Runtime"
    kind "ConsoleApp"
    language "c++"
    cppdialect "c++20"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    vectorextensions "AVX2"

    files
    {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{wks.location}/Shark-Runtime/src",
        
        "%{glm.IncludeDir}",
        "%{fmt.IncludeDir}",
        "%{spdlog.IncludeDir}",
        "%{Box2D.IncludeDir}",
        "%{EnTT.IncludeDir}",
        "%{filewatch.IncludeDir}",
        "%{ImGui.IncludeDir}",

        "%{tracy.IncludeDir}",
    }

    links
    {
        "Shark"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "_USE_MATH_DEFINES",

        glm.Defines,
        fmt.Defines,
        ImGui.Defines,
        tracy.Defines
    }

    postbuildcommands
    {
        '{COPYFILE} "%{Assimp.Binary}" "%{cfg.targetdir}"'
    }

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