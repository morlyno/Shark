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
        "src/**.cpp"
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{wks.location}/SharkFin/src",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.EnTT}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.box2d}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.fmt}",
        "%{IncludeDir.Optick}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.Mono}"
    }

    links
    {
        "Shark"
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

    sharkfin_output_dir = "%{wks.location}/bin/%{outputdir}/%{prj.name}"

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

        postbuildcommands
        {
            '{COPY} "%{Library.Mono_dll}" "%{sharkfin_output_dir}"',
            '{COPY} "%{Symbols.Mono_dll}" "%{sharkfin_output_dir}"'
        }

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"

        postbuildcommands
        {
            '{COPY} "%{Library.Mono_dll}" "%{sharkfin_output_dir}"'
        }
