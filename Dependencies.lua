
VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["spdlog"] = "%{wks.location}/Shark/dependencies/spdlog/include"
IncludeDir["ImGui"] = "%{wks.location}/Shark/dependencies/ImGui"
IncludeDir["stb_image"] = "%{wks.location}/Shark/dependencies/stb_image"
IncludeDir["EnTT"] = "%{wks.location}/Shark/dependencies/EnTT/include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Shark/dependencies/yaml-cpp/include"
IncludeDir["box2d"] = "%{wks.location}/Shark/dependencies/box2d/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Shark/dependencies/ImGuizmo"
IncludeDir["fmt"] = "%{wks.location}/Shark/dependencies/fmt/include"
IncludeDir["Optick"] = "%{wks.location}/Shark/dependencies/Optick/src"
IncludeDir["glm"] = "%{wks.location}/Shark/dependencies/glm"
IncludeDir["Mono"] = "%{wks.location}/Shark/dependencies/mono/include"
IncludeDir["msdfgen"] = "%{wks.location}/Shark/dependencies/msdf-atlas-gen/msdfgen"
IncludeDir["msdf_atlas_gen"] = "%{wks.location}/Shark/dependencies/msdf-atlas-gen/msdf-atlas-gen"
IncludeDir["Vulkan_SDK"] = "%{VULKAN_SDK}/Include"

LibraryDir = {}
LibraryDir["mono"] = "%{wks.location}/Shark/dependencies/mono/lib/%{cfg.buildcfg}"
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["mono"] = "%{LibraryDir.mono}/libmono-static-sgen.lib"

Library["D3D11"] = "d3d11.lib"
Library["DXGI"] = "dxgi.lib"
Library["dxguid"] = "dxguid.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"
Library["SPIRV_Cross_HLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-hlsl.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Cross_HLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-hlsld.lib"

Library["Winmm"] = "Winmm.lib"
Library["Version"] = "Version.lib"
Library["Bcrypt"] = "Bcrypt.lib"
Library["WinSock"] = "Ws2_32.lib"

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
}

filewatch = {
    IncludeDir = "%{wks.location}/Shark/dependencies/filewatch/include",
    Files = {
        "%{wks.location}/Shark/dependencies/filewatch/include/filewatch/FileWatch.hpp"
    }
}
