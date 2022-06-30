project "ScriptingCore"
    kind "SharedLib"
    language "c#"
    framework "4.8"
    staticruntime "off"

    targetdir ("%{wks.location}/SharkFin/Resources/Binaries")
    objdir ("%{wks.location}/bin-int/%{outputdir}/%{prj.name}")

    files
    {
        "Shark/**.cs"
    }

    includedirs
    {
    }

    flags
    {
        "MultiProcessorCompile"
    }
    
    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
