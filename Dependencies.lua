
VULKAN_SDK = os.getenv("VULKAN_SDK")

spdlog = {
    BaseDir = "%{wks.location}/Shark/dependencies/spdlog",
    IncludeDir = "%{spdlog.BaseDir}/include",
    Files = {
        "%{spdlog.BaseDir}/include/spdlog/*.h",
        "%{spdlog.BaseDir}/include/spdlog/cfg/*.h",
        "%{spdlog.BaseDir}/include/spdlog/cfg/*.h",
        "%{spdlog.BaseDir}/include/spdlog/details/*.h",
        "%{spdlog.BaseDir}/include/spdlog/sinks/*.h",
    }
}

ImGui = {
    BaseDir = "%{wks.location}/Shark/dependencies/ImGui",
    IncludeDir = "%{ImGui.BaseDir}"
}

stb_image = {
    BaseDir = "%{wks.location}/Shark/dependencies/stb_image",
    IncludeDir = "%{stb_image.BaseDir}",
    Files = {
        "%{stb_image.BaseDir}/stb_image.h",
        "%{stb_image.BaseDir}/stb_image.cpp"
    }
}

EnTT = {
    BaseDir = "%{wks.location}/Shark/dependencies/EnTT",
    IncludeDir = "%{EnTT.BaseDir}/include",
    Files = {
        "%{EnTT.BaseDir}/include/entt.hpp"
    }
}

yaml_cpp = {
    BaseDir = "%{wks.location}/Shark/dependencies/yaml-cpp",
    IncludeDir = "%{yaml_cpp.BaseDir}/include",
    Defines = { "YAML_CPP_STATIC_DEFINE" }
}

Box2D = {
    BaseDir = "%{wks.location}/Shark/dependencies/box2d",
    IncludeDir = "%{Box2D.BaseDir}/include"
}

ImGuizmo = {
    BaseDir = "%{wks.location}/Shark/dependencies/ImGuizmo",
    IncludeDir = "%{ImGuizmo.BaseDir}"
}

fmt = {
    BaseDir = "%{wks.location}/Shark/dependencies/fmt",
    IncludeDir = "%{fmt.BaseDir}/include",
    Files = { "%{fmt.BaseDir}/include/fmt/*.h" },
    Defines = { "FMT_HEADER_ONLY" }
}

glm = {
    BaseDir = "%{wks.location}/Shark/dependencies/glm",
    IncludeDir = "%{glm.BaseDir}",
    Files = {
        "%{glm.BaseDir}/glm/**.h",
        "%{glm.BaseDir}/glm/**.hpp",
        "%{glm.BaseDir}/glm/**.inl"
    },
    Defines = {
        "GLM_FORCE_SWIZZLE",
        "GLM_FORCE_LEFT_HANDED",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_INTRINSICS"
    }
}

mono = {
    BaseDir = "%{wks.location}/Shark/dependencies/mono",
    IncludeDir = "%{mono.BaseDir}/include",
    Library = "%{mono.BaseDir}/lib/%{cfg.buildcfg}/libmono-static-sgen.lib",
    Files = {
        "%{mono.BaseDir}/include/mono/**.h"
    }
}

msdf_atlas_gen = {
    BaseDir = "%{wks.location}/Shark/dependencies/msdf-atlas-gen",
    IncludeDir = "%{msdf_atlas_gen.BaseDir}"
}

msdfgen = {
    IncludeDir = "%{msdf_atlas_gen.BaseDir}/msdfgen"
}

Vulkan = {
    IncludeDir = "%{VULKAN_SDK}/Include",
}

ShaderC = {
    LibraryDebug = "%{VULKAN_SDK}/lib/shaderc_sharedd.lib",
    LibraryRelease = "%{VULKAN_SDK}/lib/shaderc_shared.lib",
    --Library = iif("%{cfg.Configuration}" == "Debug", LibraryDebug, LibraryRelease)
}

DXC = {
    LibraryDebug = "%{VULKAN_SDK}/lib/dxcompilerd.lib",
    LibraryRelease = "%{VULKAN_SDK}/lib/dxcompiler.lib"
}

SPIRV_Cross = {
    LibraryDebug = "%{VULKAN_SDK}/lib/spirv-cross-cored.lib",
    LibraryRelease = "%{VULKAN_SDK}/lib/spirv-cross-core.lib"
}

SPIRV_Cross_GLSL = {
    LibraryDebug = "%{VULKAN_SDK}/lib/spirv-cross-glsld.lib",
    LibraryRelease = "%{VULKAN_SDK}/lib/spirv-cross-glsl.lib"
}

SPIRV_Cross_HLSL = {
    LibraryDebug = "%{VULKAN_SDK}/lib/spirv-cross-hlsld.lib",
    LibraryRelease = "%{VULKAN_SDK}/lib/spirv-cross-hlsl.lib"
}

Assimp = {
    BaseDir = "%{wks.location}/Shark/dependencies/Assimp",
    IncludeDir = "%{Assimp.BaseDir}/include",
    Library = "%{Assimp.BaseDir}/lib/assimp-vc143-mt.lib",
    Binary = "%{Assimp.BaseDir}/bin/assimp-vc143-mt.dll",
}

tracy = {
    BaseDir = "%{wks.location}/Shark/dependencies/tracy",
    IncludeDir = "%{tracy.BaseDir}/public",
    Files = {
        "%{tracy.IncludeDir}/tracy/*.hpp",
        "%{tracy.IncludeDir}/libbacktrace/*.hpp",
        "%{tracy.IncludeDir}/common/*.hpp",
        "%{tracy.IncludeDir}/client/*.hpp",
        "%{tracy.IncludeDir}/TracyClient.cpp"
    },
    Defines = {
        "TRACY_ENABLE",
        "TRACY_ON_DEMAND"
    }
}

filewatch = {
    IncludeDir = "%{wks.location}/Shark/dependencies/filewatch/include",
    Files = {
        "%{wks.location}/Shark/dependencies/filewatch/include/filewatch/FileWatch.hpp"
    }
}

magic_enum = {
    IncludeDir = "%{wks.location}/Shark/dependencies/magic_enum",
    Files = {
        "%{wks.location}/Shark/dependencies/magic_enum/magic_enum.hpp"
    }
}

DirectX = {
    D3D11 = { Library = "d3d11.lib" },
    DXGI = { Library = "dxgi.lib" },
    dxguid = { Library = "dxguid.lib" }
}

Windows = {
    Winmm = { Library = "Winmm.lib" },
    Version = { Library = "Version.lib" },
    Bcrypt = { Library = "Bcrypt.lib" },
    WinSock = { Library = "Ws2_32.lib" }
}
