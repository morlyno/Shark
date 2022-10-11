workspace "%PROJECT_NAME%"
    startproject "%PROJECT_NAME%"

    configurations
    {
        "Debug",
        "Release"
    }

SharkDir = os.getenv("SHARK_DIR")

project "%PROJECT_NAME%"
    kind "SharedLib"
    language "c#"
    framework "4.7.2"
    location "Assets/Scripts"

    targetdir "%{wks.location}/Binaries"
    objdir "%{wks.location}/Intermediates"

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