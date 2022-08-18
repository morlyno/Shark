workspace "Sandbox"
    startproject "Sandbox"

    configurations
    {
        "Debug",
        "Release"
    }

SharkDir = os.getenv("SHARK_DIR")

project "Sandbox"
    kind "SharedLib"
    language "c#"
    framework "4.7.2"
    location "Assets/Scripts"

    targetdir "%{wks.location}/Assets/Scripts/Binaries"
    objdir "%{wks.location}/Assets/Scripts/Intermediates"

    files
    {
        "%{prj.location}/Source/**.cs"
    }
    
    links
    {
        "Shark-ScriptCore"
    }
    
    filter "system:windows"
        systemversion "latest"

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
        location (SharkDir .. "/Shark-ScriptCore")
group ""