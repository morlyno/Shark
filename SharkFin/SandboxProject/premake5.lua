workspace "Sandbox"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
SharkDir = os.getenv("SHARK_DIR")

project "Sandbox"
    kind "SharedLib"
    language "c#"
    framework "4.7.2"
    location "Assets/Scripts"

    targetdir "%{wks.location}/Binaries"
    objdir "%{wks.location}/Binaries/Intermediates"

    files
    {
        "%{prj.location}/**.cs"
    }
    
    links
    {
        "Shark-ScriptCore"
    }
    
    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        runtime "Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        symbols "Default"

group "Shark"
    externalproject "Shark-ScriptCore"
        kind "SharedLib"
        language "c#"
        location "%{SharkDir}/Shark-ScriptCore"