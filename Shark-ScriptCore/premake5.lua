project "Shark-ScriptCore"
    kind "SharedLib"
    language "c#"
    dotnetframework "4.7.2"

    targetdir ("%{wks.location}/Shark-Editor/Resources/Binaries")
    objdir ("%{wks.location}/Shark-Editor/Resources/Binaries/Intermediates")

    files
    {
        "Shark-ScriptCore/**.cs"
    }
