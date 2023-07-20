project "Shark"
    kind "StaticLib"
    language "c++"
    cppdialect "c++17"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skpch.h"
    pchsource "src/skpch.cpp"

    vectorextensions "AVX2"

    files
    {
        "src/**.h",
        "src/**.cpp",
        "%{IncludeDir.stb_image}/*.h",
        "%{IncludeDir.stb_image}/*.cpp",
        "%{IncludeDir.glm}/**.h",
        "%{IncludeDir.glm}/**.hpp",
        "%{IncludeDir.glm}/**.inl",
        "%{IncludeDir.fmt}/**.h",
        "%{IncludeDir.spdlog}/**.h",
        "%{IncludeDir.Mono}/**.h"
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
        "%{IncludeDir.Mono}",
        "%{IncludeDir.msdfgen}",
        "%{IncludeDir.msdf_atlas_gen}",
        "%{IncludeDir.Vulkan_SDK}",
        "%{Assimp.IncludeDir}"
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
        "msdf-atlas-gen",
        "%{Library.mono}",
        "%{Assimp.Library}"
    }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

        links
        {
            "%{Library.D3D11}",
            "%{Library.DXGI}",
            "%{Library.dxguid}",
            "%{Library.Winmm}",
            "%{Library.Version}",
            "%{Library.Bcrypt}",
            "%{Library.WinSock}"
        }

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        optimize "Off"
        symbols "Default"

        links
        {
            "%{Library.ShaderC_Debug}",
            "%{Library.SPIRV_Cross_Debug}",
            "%{Library.SPIRV_Cross_GLSL_Debug}",
            "%{Library.SPIRV_Cross_HLSL_Debug}",
        }

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "On"
        symbols "Default"
        
        links
        {
            "%{Library.ShaderC_Release}",
            "%{Library.SPIRV_Cross_Release}",
            "%{Library.SPIRV_Cross_GLSL_Release}",
            "%{Library.SPIRV_Cross_HLSL_Release}",
        }
