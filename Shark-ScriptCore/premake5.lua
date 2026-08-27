SharkDir = os.getenv("SHARK_DIR")

include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Coral.Managed"))

project "Shark-ScriptCore"
    tags { "Shark" }
    kind "SharedLib"
    language "C#"
    dotnetframework "net9.0"
    clr "Unsafe"

    targetdir ("%{SharkDir}/Shark-Editor/Resources/Binaries")
    objdir ("%{SharkDir}/Shark-Editor/Resources/Binaries/Intermediates")

    links { "Coral.Managed" }

    vsprops {
        AppendTargetFrameworkToOutputPath = "false",
        Nullable = "enable",
        CopyLocalLockFileAssemblies = "true",
        EnableDynamicLoading = "true"
    }

    files {
        "Source/**.cs"
    }
    
    filter { "system:windows" }
        postbuildcommands {
            '"%{wks.location}Scripts/CopyDotNet.bat" "%{cfg.buildcfg}"'
        }
