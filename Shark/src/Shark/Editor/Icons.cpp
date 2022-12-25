#include "skpch.h"
#include "Icons.h"
#include "Shark\Core\Timer.h"
#include "Shark\Serialization\TextureSerializers.h"

namespace Shark {

	Ref<Image2D> Icons::SettingsIcon;
	Ref<Image2D> Icons::FileIcon;
	Ref<Image2D> Icons::FolderIcon;
	Ref<Image2D> Icons::PNGIcon;
	Ref<Image2D> Icons::SceneIcon;
	Ref<Image2D> Icons::ScriptIcon;
	Ref<Image2D> Icons::TextureIcon;
	Ref<Image2D> Icons::InfoIcon;
	Ref<Image2D> Icons::WarnIcon;
	Ref<Image2D> Icons::ErrorIcon;
	Ref<Image2D> Icons::PlayIcon;
	Ref<Image2D> Icons::StopIcon;
	Ref<Image2D> Icons::PauseIcon;
	Ref<Image2D> Icons::SimulateIcon;
	Ref<Image2D> Icons::StepIcon;

	void Icons::Init()
	{
		SK_CORE_INFO("Loading Icons...");
		ScopedTimer timer("Loading Icons");

		SettingsIcon  = Image2D::LoadFromDisc("Resources/Icon_Settings.png");

		FileIcon      = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_File.png");
		FolderIcon    = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_Folder.png");
		PNGIcon       = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_PNG.png");
		SceneIcon     = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_Scene.png");
		ScriptIcon    = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_Script.png");
		TextureIcon   = Image2D::LoadFromDisc("Resources/ContentBrowser/Icon_Texture.png");

		InfoIcon      = Image2D::LoadFromDisc("Resources/Console/Icon_Info.png");
		WarnIcon      = Image2D::LoadFromDisc("Resources/Console/Icon_Warn.png");
		ErrorIcon     = Image2D::LoadFromDisc("Resources/Console/Icon_Error.png");
		
		PlayIcon      = Image2D::LoadFromDisc("Resources/Toolbar/Icon_Play.png");
		StopIcon      = Image2D::LoadFromDisc("Resources/Toolbar/Icon_Stop.png");
		PauseIcon     = Image2D::LoadFromDisc("Resources/Toolbar/Icon_Pause.png");
		SimulateIcon  = Image2D::LoadFromDisc("Resources/Toolbar/Icon_Simulate.png");
		StepIcon      = Image2D::LoadFromDisc("Resources/Toolbar/Icon_Step.png");
	}

	void Icons::Shutdown()
	{
		SettingsIcon  = nullptr;

		FileIcon      = nullptr;
		FolderIcon    = nullptr;
		PNGIcon       = nullptr;
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
	}

	static void ReloadImageFromDisc(Ref<Image2D> image, const std::filesystem::path& filepath)
	{
		ImageSerializer serializer(image);
		serializer.Deserialize(filepath);
	}

	void Icons::Reload()
	{
		ReloadImageFromDisc(SettingsIcon, "Resources/Icon_Settings.png");
		ReloadImageFromDisc(FileIcon,     "Resources/ContentBrowser/Icon_File.png");
		ReloadImageFromDisc(FolderIcon,   "Resources/ContentBrowser/Icon_Folder.png");
		ReloadImageFromDisc(PNGIcon,      "Resources/ContentBrowser/Icon_PNG.png");
		ReloadImageFromDisc(SceneIcon,    "Resources/ContentBrowser/Icon_Scene.png");
		ReloadImageFromDisc(ScriptIcon,   "Resources/ContentBrowser/Icon_Script.png");
		ReloadImageFromDisc(TextureIcon,  "Resources/ContentBrowser/Icon_Texture.png");
		ReloadImageFromDisc(InfoIcon,     "Resources/Console/Icon_Info.png");
		ReloadImageFromDisc(WarnIcon,     "Resources/Console/Icon_Warn.png");
		ReloadImageFromDisc(ErrorIcon,    "Resources/Console/Icon_Error.png");
		ReloadImageFromDisc(PlayIcon,     "Resources/Toolbar/Icon_Play.png");
		ReloadImageFromDisc(StopIcon,     "Resources/Toolbar/Icon_Stop.png");
		ReloadImageFromDisc(PauseIcon,    "Resources/Toolbar/Icon_Pause.png");
		ReloadImageFromDisc(SimulateIcon, "Resources/Toolbar/Icon_Simulate.png");
		ReloadImageFromDisc(StepIcon,     "Resources/Toolbar/Icon_Step.png");
	}

}
