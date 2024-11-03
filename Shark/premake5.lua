project "Shark"
    kind "StaticLib"

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "skpch.h"
    pchsource "src/skpch.cpp"

    dependson { "Shark-ScriptCore" }

    files {
        "src/**.h",
        "src/**.cpp",

        "dependencies/stb_image/stb_image.h",
        "dependencies/stb_image/stb_image.cpp",
        
        "dependencies/ImGuizmo/ImGuizmo.h",
        "dependencies/ImGuizmo/ImGuizmo.cpp",

        "dependencies/tracy/tracy/*.hpp",
        "dependencies/tracy/libbacktrace/*.hpp",
        "dependencies/tracy/common/*.hpp",
        "dependencies/tracy/client/*.hpp",
        "dependencies/tracy/TracyClient.cpp",
    }

    includedirs {
        "src/",
        "dependencies/"
    }

    defines {
        "GLM_FORCE_LEFT_HANDED",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
        "GLM_FORCE_SWIZZLE"
    }

    IncludeDependencies()

    filter "files:dependencies/tracy/**.cpp or files:dependencies/ImGuizmo/ImGuizmo.cpp"
        flags { "NoPCH" }

    filter "files:dependencies/StrToNum/StrToNum.h"
        defines { "NDEBUG" }
    
        
    filter "configurations:Debug or configurations:Debug-AS or configurations:Release"
        defines { "SK_TRACK_MEMORY" }


    filter "configurations:Debug or configurations:Debug-AS"
        LinkDependencies("Debug")
        
    filter "configurations:Release"
        LinkDependencies("Release")
