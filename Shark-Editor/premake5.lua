project "Shark-Editor"
    tags { "Shark" }
    kind "ConsoleApp"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    links { "Shark" }

    files {
        "src/**.h",
        "src/**.cpp",

        "Resources/Shaders/**.hlsl",
        "Resources/Shaders/**.hlslh",
        "Resources/Shaders/**.glsl",
        "Resources/Shaders/**.glslh"
    }

    includedirs {
        "src/",

        "%{wks.location}/Shark/src/",
        "%{wks.location}/Shark/dependencies/"
    }

    IncludeDependencies();
    PostbuildNethost();

    defines {
        "GLM_FORCE_LEFT_HANDED",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_SWIZZLE"
    }

    postbuildcommands {
        '{COPY} "%{Dependencies.Assimp.DLL}" "%{cfg.targetdir}"'
    }

    filter "files:**.hlsl or files:**.glsl"
        excludefrombuild "On"

    filter "configurations:Debug or configurations:Debug-AS or configurations:Release"
        defines { "SK_TRACK_MEMORY" }
