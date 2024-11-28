SharkDir = os.getenv("SHARK_DIR")
include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Premake", "CSExtensions.lua"))
include (path.join(SharkDir, "Shark", "dependencies", "Coral", "Coral.Managed"))

project "Shark-ScriptCore"
    kind "SharedLib"
    language "C#"
    dotnetframework "net8.0"
    clr "Unsafe"

    targetdir ("%{SharkDir}/Shark-Editor/Resources/Binaries")
    objdir ("%{SharkDir}/Shark-Editor/Resources/Binaries/Intermediates")

    links { "Coral.Managed" }

    propertytags {
        { "AppendTargetFrameworkToOutputPath", "false" },
        { "Nullable", "enable" }
    }

    files {
        "Source/**.cs"
    }
    
    filter { "system:windows" }
        postbuildcommands {
            '"%{wks.location}Scripts/CopyDotNet.bat" "%{cfg.buildcfg}"'
        }
