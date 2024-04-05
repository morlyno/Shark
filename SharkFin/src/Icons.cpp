#include "skfpch.h"
#include "Icons.h"

#include "Shark/Core/Timer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/Import/TextureImporter.h"

namespace Shark {

	void Icons::Init()
	{
		SK_CORE_INFO("Loading Icons...");
		ScopedTimer timer("Loading Icons");

		SettingsIcon  = Texture2D::Create(TextureSpecification(), "Resources/Icon_Settings.png");

		FileIcon      = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_File.png");
		FolderIcon    = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_Folder.png");
		PNGIcon       = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_PNG.png");
		JPGIcon       = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_JPG.png");
		SceneIcon     = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_Scene.png");
		ScriptIcon    = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_Script.png");
		TextureIcon   = Texture2D::Create(TextureSpecification(), "Resources/ContentBrowser/Icon_Texture.png");

		InfoIcon      = Texture2D::Create(TextureSpecification(), "Resources/Console/Icon_Info.png");
		WarnIcon      = Texture2D::Create(TextureSpecification(), "Resources/Console/Icon_Warn.png");
		ErrorIcon     = Texture2D::Create(TextureSpecification(), "Resources/Console/Icon_Error.png");
		
		PlayIcon      = Texture2D::Create(TextureSpecification(), "Resources/Toolbar/Icon_Play.png");
		StopIcon      = Texture2D::Create(TextureSpecification(), "Resources/Toolbar/Icon_Stop.png");
		PauseIcon     = Texture2D::Create(TextureSpecification(), "Resources/Toolbar/Icon_Pause.png");
		SimulateIcon  = Texture2D::Create(TextureSpecification(), "Resources/Toolbar/Icon_Simulate.png");
		StepIcon      = Texture2D::Create(TextureSpecification(), "Resources/Toolbar/Icon_Step.png");

		ClearIcon = Texture2D::Create(TextureSpecification(), "Resources/Icons/ClearIcon.png");
		ReloadIcon = Texture2D::Create(TextureSpecification(), "Resources/Icons/ReloadIcon.png");
	}

	void Icons::Shutdown()
	{
		SettingsIcon  = nullptr;

		FileIcon      = nullptr;
		FolderIcon    = nullptr;
		PNGIcon       = nullptr;
		JPGIcon       = nullptr;
		SceneIcon     = nullptr;
		ScriptIcon    = nullptr;
		TextureIcon   = nullptr;

		InfoIcon      = nullptr;
		WarnIcon      = nullptr;
		ErrorIcon     = nullptr;

		PlayIcon      = nullptr;
		StopIcon      = nullptr;
		PauseIcon     = nullptr;
		SimulateIcon  = nullptr;
		StepIcon      = nullptr;

		ClearIcon = nullptr;
		ReloadIcon = nullptr;
	}

	static void ReloadIconFromDisc(Ref<Texture2D> icon, const std::filesystem::path& filepath)
	{
		Buffer& imageData = icon->GetBuffer();
		imageData.Release();

		TextureSpecification& specification = icon->GetSpecification();
		imageData = TextureImporter::ToBufferFromFile(filepath, specification.Format, specification.Width, specification.Height);

		icon->Invalidate();
	}

	void Icons::Reload()
	{
		ReloadIconFromDisc(SettingsIcon, "Resources/Icon_Settings.png");
		ReloadIconFromDisc(FileIcon,     "Resources/ContentBrowser/Icon_File.png");
		ReloadIconFromDisc(FolderIcon,   "Resources/ContentBrowser/Icon_Folder.png");
		ReloadIconFromDisc(PNGIcon,      "Resources/ContentBrowser/Icon_PNG.png");
		ReloadIconFromDisc(JPGIcon,      "Resources/ContentBrowser/Icon_JPG.png");
		ReloadIconFromDisc(SceneIcon,    "Resources/ContentBrowser/Icon_Scene.png");
		ReloadIconFromDisc(ScriptIcon,   "Resources/ContentBrowser/Icon_Script.png");
		ReloadIconFromDisc(TextureIcon,  "Resources/ContentBrowser/Icon_Texture.png");
		ReloadIconFromDisc(InfoIcon,     "Resources/Console/Icon_Info.png");
		ReloadIconFromDisc(WarnIcon,     "Resources/Console/Icon_Warn.png");
		ReloadIconFromDisc(ErrorIcon,    "Resources/Console/Icon_Error.png");
		ReloadIconFromDisc(PlayIcon,     "Resources/Toolbar/Icon_Play.png");
		ReloadIconFromDisc(StopIcon,     "Resources/Toolbar/Icon_Stop.png");
		ReloadIconFromDisc(PauseIcon,    "Resources/Toolbar/Icon_Pause.png");
		ReloadIconFromDisc(SimulateIcon, "Resources/Toolbar/Icon_Simulate.png");
		ReloadIconFromDisc(StepIcon,     "Resources/Toolbar/Icon_Step.png");

		ReloadIconFromDisc(ClearIcon,    "Resources/Toolbar/ClearIcon.png");
		ReloadIconFromDisc(ReloadIcon,   "Resources/Toolbar/ReloadIcon.png");
	}

}
