project "Shark-ScriptCore"
    kind "SharedLib"
    language "c#"
    dotnetframework "4.7.2"

    targetdir ("%{wks.location}/SharkFin/Resources/Binaries")
    objdir ("%{wks.location}/SharkFin/Resources/Binaries/Intermediates")

    files
    {
        "Shark-ScriptCore/**.cs"
    }

    includedirs
    {
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
