
SharkDir = os.getenv("SHARK_DIR")
include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Premake", "CSExtensions.lua"))

workspace "Sandbox"
    configurations { "Debug", "Debug-AS", "Release" }
    startproject "Sandbox"


group "Shark"
    include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Coral.Managed"))
    include (path.join(SharkDir, "Shark-ScriptCore"))
group ""


project "Sandbox"
    location "Assets/Scripts"
    kind "SharedLib"
    language "C#"
    framework "net8.0"

    targetname "Sandbox"
    targetdir "%{prj.location}/Binaries"
    objdir "%{prj.location}/Intermediates"

    propertytags {
        { "AppendTargetFrameworkToOutputPath", "false" },
        { "Nullable", "enable" }
    }

    files {
        "%{prj.location}/Source/**.cs"
    }
    
    links {
        "Shark-ScriptCore"
    }
    
    filter "configurations:Debug"
        optimize "Off"
        symbols "Default"

    filter "configurations:Release"
        optimize "On"
        symbols "Default"
