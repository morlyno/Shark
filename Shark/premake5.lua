project "Shark"
    kind "StaticLib"
    language "c++"
    cppdialect "c++20"
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

        stb_image.Files,
        glm.Files,
        fmt.Files,
        spdlog.Files,
        tracy.Files,
        filewatch.Files,
        magic_enum.Files
    }

    includedirs
    {
        "%{wks.location}/Shark/src",
        "%{spdlog.IncludeDir}",
        "%{ImGui.IncludeDir}",
        "%{stb_image.IncludeDir}",
        "%{EnTT.IncludeDir}",
        "%{yaml_cpp.IncludeDir}",
        "%{Box2D.IncludeDir}",
        "%{ImGuizmo.IncludeDir}",
        "%{fmt.IncludeDir}",
        "%{glm.IncludeDir}",
        "%{mono.IncludeDir}",
        "%{msdf_atlas_gen.IncludeDir}",
        "%{msdfgen.IncludeDir}",
        "%{Vulkan.IncludeDir}",
        "%{Assimp.IncludeDir}",
        "%{tracy.IncludeDir}",
        "%{filewatch.IncludeDir}",
        "%{magic_enum.IncludeDir}"
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
        "msdf-atlas-gen",
        "%{mono.Library}",
        "%{Assimp.Library}"
    }

    filter "files:dependencies/tracy/**.cpp"
        flags { "NoPCH" }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

        links
        {
            "%{DirectX.D3D11.Library}",
            "%{DirectX.DXGI.Library}",
            "%{DirectX.dxguid.Library}",

            "%{Windows.Winmm.Library}",
            "%{Windows.Version.Library}",
            "%{Windows.Bcrypt.Library}",
            "%{Windows.WinSock.Library}",
        }

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        optimize "Off"
        symbols "Default"

        links
        {
            "%{DXC.LibraryDebug}",
            "%{ShaderC.LibraryDebug}",
            "%{SPIRV_Cross.LibraryDebug}",
            "%{SPIRV_Cross_GLSL.LibraryDebug}",
            "%{SPIRV_Cross_HLSL.LibraryDebug}",
        }

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "On"
        symbols "Default"
        
        links
        {
            "%{DXC.LibraryRelease}",
            "%{ShaderC.LibraryRelease}",
            "%{SPIRV_Cross.LibraryRelease}",
            "%{SPIRV_Cross_GLSL.LibraryRelease}",
            "%{SPIRV_Cross_HLSL.LibraryRelease}",
        }
