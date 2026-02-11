
VULKAN_SDK = os.getenv("VULKAN_SDK")

Dependencies = {
    NVHRI = {
        LibName = "NVRHI",
        IncludeDir = "%{wks.location}/Shark/dependencies/nvrhi/include"
    },

    spdlog = {
        IncludeDir = "%{wks.location}/Shark/dependencies/spdlog/include"
    },
    ImGui = {
        LibName = "ImGui",
        IncludeDir = "%{wks.location}/Shark/dependencies/ImGui"
    },
    ImGuizmo = {
        IncludeDir = "%{wks.location}/Shark/dependencies/ImGuizmo"
    },
    stb_image = {
        IncludeDir = "%{wks.location}/Shark/dependencies/stb_image"
    },
    EnTT = {
        IncludeDir = "%{wks.location}/Shark/dependencies/EnTT/include"
    },
    yaml_cpp = {
        LibName = "yaml-cpp",
        IncludeDir = "%{wks.location}/Shark/dependencies/yaml-cpp/include"
    },
    Box2D = {
        LibName = "Box2D",
        IncludeDir = "%{wks.location}/Shark/dependencies/box2d/include"
    },
    fmt = {
        IncludeDir = "%{wks.location}/Shark/dependencies/fmt/include"
    },
    glm = {
        IncludeDir = "%{wks.location}/Shark/dependencies/glm"
    },
    Coral = {
        LibName = "Coral.Native",
        IncludeDir = "%{wks.location}/Shark/dependencies/Coral/Coral.Native/Include"
    },
    msdf_atlas_gen = {
        LibName = "msdf-atlas-gen",
        IncludeDir = "%{wks.location}/Shark/dependencies/msdf-atlas-gen"
    },
    msdfgen = {
        IncludeDir = "%{wks.location}/Shark/dependencies/msdf-atlas-gen/msdfgen"
    },
    Vulkan = {
        IncludeDir = "%{VULKAN_SDK}/Include",
    },
    ShaderC = {
        LibName = "shaderc_shared",
        DebugLibName = "shaderc_sharedd",
        LibraryDir = "%{VULKAN_SDK}/lib"
    },
    DXCompiler = {
        LibName = "dxcompiler",
        DebugLibName = "dxcompilerd",
        LibraryDir = "%{VULKAN_SDK}/lib"
    },
    SPIRV_Cross = {
        LibName = "spirv-cross-core",
        DebugLibName = "spirv-cross-cored",
        LibraryDir = "%{VULKAN_SDK}/lib"
    },
    SPIRV_Cross_GLSL = {
        LibName = "spirv-cross-glsl",
        DebugLibName = "spirv-cross-glsld",
        LibraryDir = "%{VULKAN_SDK}/lib"
    },
    SPIRV_Cross_HLSL = {
        LibName = "spirv-cross-hlsl",
        DebugLibName = "spirv-cross-hlsld",
        LibraryDir = "%{VULKAN_SDK}/lib"
    },
    Assimp = {
        LibName = "assimp-vc143-mt",
        IncludeDir = "%{wks.location}/Shark/dependencies/Assimp/include",
        LibraryDir = "%{wks.location}/Shark/dependencies/Assimp/lib",
        DLL = "%{wks.location}/Shark/dependencies/Assimp/bin/assimp-vc143-mt.dll"
    },
    tracy = {
        IncludeDir = "%{wks.location}/Shark/dependencies/tracy",
        Config = "Release"
    },
    filewatch = {
        IncludeDir = "%{wks.location}/Shark/dependencies/filewatch/include",
    },
    magic_enum = {
        IncludeDir = "%{wks.location}/Shark/dependencies/magic_enum",
    },
    StrToNum = {
        IncludeDir = "%{wks.location}/Shark/dependencies/StrToNum"
    },

    D3D11 = {
        Windows = { LibName = "d3d11" }
    },
    DXGI = {
        Windows = { LibName = "dxgi" }
    },
    dxguid = {
        Windows = { LibName = "dxguid" }
    },
    D3DCompiler = {
        Windows = { LibName = "d3dcompiler" }
    },
    Winmm = {
        Windows = { LibName = "Winmm" }
    },
    Version = {
        Windows = { LibName = "Version" }
    },
    Bcrypt = {
        Windows = { LibName = "Bcrypt" }
    },
    WinSock = {
        Windows = { LibName = "Ws2_32" }
    }
}

function AddInclude(libData)
    if libData.IncludeDir ~= nil then
        externalincludedirs { libData.IncludeDir }
    end
end

function LinkLibrary(libData, isDebug)
    if libData.LibraryDir ~= nil then
        libdirs { libData.LibraryDir }
    end

    local libraryName = nil;
    if libData.LibName ~= nil then
        libraryName = libData.LibName
    end

    if libData.DebugLibName ~= nil and isDebug then
        libraryName = libData.DebugLibName;
    end

    if libraryName ~= nil then
        links { libraryName }
        return true
    end
    return false
end

function FistToUpper(str)
    return (str:gsub("^%l", string.upper))
end

function IncludeDependencies()
    local target = FistToUpper(os.target())

    for key, libraryData in pairs(Dependencies) do
        AddInclude(libraryData)

        -- platform specific
        if libraryData[target] ~= nil then
            AddInclude(libraryData[target])
        end
    end
end

function LinkDependencies(targetConfig)
    local target = FistToUpper(os.target())

    for key, libraryData in pairs(Dependencies) do
        local matchesConfig = true
        
        if targetConfig ~= nil and libraryData.Config ~= nil then
            matchesConfig = string.find(libraryData.Config, targetConfig)
        end
        
        if matchesConfig then
            local linkFinished = false
            local isDebug = targetConfig == "Debug"
            -- link platform specific
            if libraryData[target] ~= nil then
                linkFinished = LinkLibrary(libraryData[target], isDebug)
                AddInclude(libraryData[target])
            end

            if not linkFinished then
                LinkLibrary(libraryData, isDebug)
            end

            AddInclude(libraryData)
        end
    end
end
