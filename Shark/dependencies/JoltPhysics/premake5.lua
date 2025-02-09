project "JoltPhysics"
    kind "StaticLib"
	language "c++"
	cppdialect "c++17"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "JoltPhysics/Jolt/**.h",
        "JoltPhysics/Jolt/**.cpp",
        "JoltPhysics/Jolt/**.inl",
	}

	includedirs {
		"JoltPhysics",
		--"JoltPhysics/Jolt",
	}

	filter "system:windows"
        systemversion "latest"
        
    filter "configurations:Debug or configurations:Debug-AS"
        runtime "Debug"
        symbols "on"

        defines {
            "JPH_DEBUG_RENDERER", -- Adds support to draw lines and triangles, used to be able to debug draw the state of the world.
            "JPH_ENABLE_ASSERTS", -- Compiles the library so that it rises an assert in case of failures. The library ignores these failures otherwise.
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED", -- Turns on division by zero and invalid floating point exception support in order to detect bugs (Windows only).
            --"JPH_EXTERNAL_PROFILE" -- Turns on the internal profiler but forwards the information to a user defined external system (see Profiler.h).
        }

    filter "configurations:Release"
        runtime "Release"
		optimize "on"

        defines {
            "JPH_DEBUG_RENDERER", -- Adds support to draw lines and triangles, used to be able to debug draw the state of the world.
            "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED", -- Turns on division by zero and invalid floating point exception support in order to detect bugs (Windows only).
            --"JPH_EXTERNAL_PROFILE" -- Turns on the internal profiler but forwards the information to a user defined external system (see Profiler.h).
        }