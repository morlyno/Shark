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
        "IMGUI_DEFINE_MATH_OPERATORS",
        
        "GLM_FORCE_SWIZZLE",
        "GLM_FORCE_LEFT_HANDED",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        
        "FMT_HEADER_ONLY",
        "MONO_DIRECTORY=%{MonoDir}",
        "YAML_CPP_STATIC_DEFINE"
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
            "OptickCore",
            "%{Library.Mono_lib}"
        }

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
