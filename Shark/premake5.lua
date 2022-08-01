project "Shark"
    kind "StaticLib"
    language "c++"
    cppdialect "c++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skpch.h"
    pchsource "src/skpch.cpp"

    if Settings.UseVectorExtensions then
        vectorextensions "AVX2"
    end

    files
    {
        "src/**.h",
        "src/**.cpp",
        "%{IncludeDir.stb_image}/*.h",
        "%{IncludeDir.stb_image}/*.cpp",
        "%{IncludeDir.glm}/**.h",
        "%{IncludeDir.glm}/**.inl",
        "%{IncludeDir.fmt}/**.h",
        "%{IncludeDir.spdlog}/**.h"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.EnTT}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.box2d}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.fmt}",
        "%{IncludeDir.Optick}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.Mono}"
    }

    defines
    {
        DefaultDefines
    }

    links
    {
        "ImGui",
        "yaml-cpp",
        "box2d",
        "ImGuizmo",
        "OptickCore",
        "%{Library.mono}"
    }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

        links
        {
            "%{Library.D3D11}",
            "%{Library.DXGI}",
            "%{Library.Winmm}",
            "%{Library.Version}",
            "%{Library.Bcrypt}"
        }

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
