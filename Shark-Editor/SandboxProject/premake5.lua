
SharkDir = os.getenv("SHARK_DIR")

workspace "Sandbox"
    configurations { "Debug", "Release" }
    startproject "Sandbox"


group "Shark"
    include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Coral.Managed"))
    include (path.join(SharkDir, "Shark-ScriptCore"))
group ""


project "Sandbox"
    location "Assets/Scripts"
    kind "SharedLib"
    language "C#"
    framework "net9.0"

    targetname "Sandbox"
    targetdir "%{prj.location}/Binaries"
    objdir "%{prj.location}/Intermediates"

    vsprops {
        AppendTargetFrameworkToOutputPath = "false",
        Nullable = "enable",
        CopyLocalLockFileAssemblies = "true",
        EnableDynamicLoading = "true",
        RollForward = "Major",
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
