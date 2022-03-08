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
        "%{includeDir.stb_image}/*.cpp",
        "%{includeDir.glm}/**.h",
        "%{includeDir.glm}/**.inl",
        "%{includeDir.fmt}/**.h",
        "%{includeDir.spdlog}/**.h"
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
        "%{includeDir.fmt}",
        "%{includeDir.Optick}",
        "%{includeDir.glm}"
    }

    flags
    {
        "MultiProcessorCompile"
    }

    defines
    {
        "IMGUI_DEFINE_MATH_OPERATORS",
        
        "GLM_FORCE_SWIZZLE",
        "GLM_FORCE_LEFT_HANDED",
        
        "FMT_HEADER_ONLY"
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
            "OptickCore"
        }

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
