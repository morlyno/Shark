project "Sandbox"
    kind "ConsoleApp"
    language "c++"
    cppdialect "c++17"
    staticruntime "on"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.EnTT}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.box2d}"
    }

    links
    {
        "Shark"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
