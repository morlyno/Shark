workspace "Sandbox"
    architecture "x64"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
SharkDir = "C:/dev/c++/shark-scripting"

project "Sandbox"
    kind "SharedLib"
    language "c#"
    framework "4.8"
    staticruntime "off"
    location "Assets/Scripts"

    targetdir "%{wks.location}/bin/%{outputdir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{outputdir}/%{prj.name}"

    files
    {
        "%{prj.location}/**.cs"
    }

    includedirs
    {
    }

    links
    {
        "ScriptingCore"
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

group "Shark"
    externalproject "ScriptingCore"
        kind "SharedLib"
        language "c#"
        location "%{SharkDir}/ScriptingCore"