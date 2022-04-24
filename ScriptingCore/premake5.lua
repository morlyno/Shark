project "ScriptingCore"
    kind "SharedLib"
    language "c#"
    framework "4.8"
    staticruntime "off"

    targetdir ("%{wks.location}/bin/%{outputdir}/%{prj.name}")
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
    
    postBuildCopySource = "%{prj.location}/../bin/%{outputdir}/%{prj.name}/%{prj.name}"
    postBuildCopyTargetDir = "%{prj.location}/../SharkFin/Resources/Binaries"

    postbuildcommands
    {
        '{ECHO} "Copying ScriptingCore..."',
        '{ECHO} "Copy: %{postBuildCopySource}.dll => %{postBuildCopyTargetDir}"',
        '{COPY} %{postBuildCopySource}.dll %{postBuildCopyTargetDir}'
    }

    filter "system:windows"
        systemversion "latest"
        defines "SK_PLATFORM_WINDOWS"

    filter "configurations:Debug"
        defines "SK_DEBUG"
        runtime "Debug"
        symbols "on"

        postbuildcommands
        {
            '{ECHO} "Copy: %{postBuildCopySource}.pdb => %{postBuildCopyTargetDir}"',
            '{COPY} %{postBuildCopySource}.pdb %{postBuildCopyTargetDir}'
        }
    
    filter "configurations:Release"
        defines "SK_RELEASE"
        runtime "Release"
        optimize "on"
        
    filter {}

    postbuildcommands
    {
        '{ECHO} "Done"'
    }
