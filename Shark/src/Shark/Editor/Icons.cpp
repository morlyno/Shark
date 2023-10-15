#include "skpch.h"
#include "Icons.h"

#include "Shark\Core\Timer.h"
#include "Shark\File\FileSystem.h"
#include "Shark\Serialization\TextureSerializers.h"

namespace Shark {

	Ref<Texture2D> Icons::SettingsIcon;
	Ref<Texture2D> Icons::FileIcon;
	Ref<Texture2D> Icons::FolderIcon;
	Ref<Texture2D> Icons::PNGIcon;
	Ref<Texture2D> Icons::SceneIcon;
	Ref<Texture2D> Icons::ScriptIcon;
	Ref<Texture2D> Icons::TextureIcon;
	Ref<Texture2D> Icons::InfoIcon;
	Ref<Texture2D> Icons::WarnIcon;
	Ref<Texture2D> Icons::ErrorIcon;
	Ref<Texture2D> Icons::PlayIcon;
	Ref<Texture2D> Icons::StopIcon;
	Ref<Texture2D> Icons::PauseIcon;
	Ref<Texture2D> Icons::SimulateIcon;
	Ref<Texture2D> Icons::StepIcon;

	void Icons::Init()
	{
		SK_CORE_INFO("Loading Icons...");
		ScopedTimer timer("Loading Icons");

		SettingsIcon  = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Icon_Settings.png"));

		FileIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_File.png"));
		FolderIcon    = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Folder.png"));
		PNGIcon       = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_PNG.png"));
		SceneIcon     = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Scene.png"));
		ScriptIcon    = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Script.png"));
		TextureIcon   = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Texture.png"));

		InfoIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Console/Icon_Info.png"));
		WarnIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Console/Icon_Warn.png"));
		ErrorIcon     = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Console/Icon_Error.png"));
		
		PlayIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Toolbar/Icon_Play.png"));
		StopIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Toolbar/Icon_Stop.png"));
		PauseIcon     = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Toolbar/Icon_Pause.png"));
		SimulateIcon  = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Toolbar/Icon_Simulate.png"));
		StepIcon      = Texture2D::LoadFromDisc(FileSystem::GetResourcePath("Resources/Toolbar/Icon_Step.png"));
	}

	void Icons::InitWithDummyImages()
	{
		SettingsIcon  = Texture2D::Create();
		FileIcon      = Texture2D::Create();
		FolderIcon    = Texture2D::Create();
		PNGIcon       = Texture2D::Create();
		SceneIcon     = Texture2D::Create();
		ScriptIcon    = Texture2D::Create();
		TextureIcon   = Texture2D::Create();
		InfoIcon      = Texture2D::Create();
		WarnIcon      = Texture2D::Create();
		ErrorIcon     = Texture2D::Create();
		PlayIcon      = Texture2D::Create();
		StopIcon      = Texture2D::Create();
		PauseIcon     = Texture2D::Create();
		SimulateIcon  = Texture2D::Create();
		StepIcon      = Texture2D::Create();
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

	static void ReloadIconFromDisc(Ref<Texture2D> icon, const std::filesystem::path& filepath)
	{
		Ref<TextureSource> source = Ref<TextureSource>::Create();
		TextureSourceSerializer serializer;
		serializer.Deserialize(source, filepath);
		icon->SetTextureSource(source);
		icon->Invalidate();
	}

	void Icons::Reload()
	{
		ReloadIconFromDisc(SettingsIcon, FileSystem::GetResourcePath("Resources/Icon_Settings.png"));
		ReloadIconFromDisc(FileIcon,     FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_File.png"));
		ReloadIconFromDisc(FolderIcon,   FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Folder.png"));
		ReloadIconFromDisc(PNGIcon,      FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_PNG.png"));
		ReloadIconFromDisc(SceneIcon,    FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Scene.png"));
		ReloadIconFromDisc(ScriptIcon,   FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Script.png"));
		ReloadIconFromDisc(TextureIcon,  FileSystem::GetResourcePath("Resources/ContentBrowser/Icon_Texture.png"));
		ReloadIconFromDisc(InfoIcon,     FileSystem::GetResourcePath("Resources/Console/Icon_Info.png"));
		ReloadIconFromDisc(WarnIcon,     FileSystem::GetResourcePath("Resources/Console/Icon_Warn.png"));
		ReloadIconFromDisc(ErrorIcon,    FileSystem::GetResourcePath("Resources/Console/Icon_Error.png"));
		ReloadIconFromDisc(PlayIcon,     FileSystem::GetResourcePath("Resources/Toolbar/Icon_Play.png"));
		ReloadIconFromDisc(StopIcon,     FileSystem::GetResourcePath("Resources/Toolbar/Icon_Stop.png"));
		ReloadIconFromDisc(PauseIcon,    FileSystem::GetResourcePath("Resources/Toolbar/Icon_Pause.png"));
		ReloadIconFromDisc(SimulateIcon, FileSystem::GetResourcePath("Resources/Toolbar/Icon_Simulate.png"));
		ReloadIconFromDisc(StepIcon,     FileSystem::GetResourcePath("Resources/Toolbar/Icon_Step.png"));
	}

}
