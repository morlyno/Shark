project "Shark-Runtime"
    kind "ConsoleApp"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    links { "Shark" }

    files {
        "src/**.h",
        "src/**.cpp"
    }

    includedirs {
        "src/",

        "%{wks.location}/Shark/src",
        "%{wks.location}/Shark/dependencies",
    }

    defines {
        "GLM_FORCE_LEFT_HANDED",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_SWIZZLE"
    }

    IncludeDependencies();

    postbuildcommands {
        '{COPYFILE} "%{Dependencies.Assimp.DLL}" "%{cfg.targetdir}"'
    }
