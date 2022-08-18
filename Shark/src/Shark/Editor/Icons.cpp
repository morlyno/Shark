#include "skpch.h"
#include "Icons.h"

namespace Shark {

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
		FileIcon      = Image2D::Create("Resources/ContentBrowser/Icon_File.png");
		FolderIcon    = Image2D::Create("Resources/ContentBrowser/Icon_Folder.png");
		PNGIcon       = Image2D::Create("Resources/ContentBrowser/Icon_PNG.png");
		SceneIcon     = Image2D::Create("Resources/ContentBrowser/Icon_Scene.png");
		ScriptIcon    = Image2D::Create("Resources/ContentBrowser/Icon_Script.png");
		TextureIcon   = Image2D::Create("Resources/ContentBrowser/Icon_Texture.png");

		InfoIcon      = Image2D::Create("Resources/Console/Icon_Info.png");
		WarnIcon      = Image2D::Create("Resources/Console/Icon_Warn.png");
		ErrorIcon     = Image2D::Create("Resources/Console/Icon_Error.png");
		
		PlayIcon      = Image2D::Create("Resources/Toolbar/Icon_Play.png");
		StopIcon      = Image2D::Create("Resources/Toolbar/Icon_Stop.png");
		PauseIcon     = Image2D::Create("Resources/Toolbar/Icon_Stop.png");
		SimulateIcon  = Image2D::Create("Resources/Toolbar/Icon_Simulate.png");
		StepIcon      = Image2D::Create("Resources/Toolbar/Icon_Step.png");
	}

	void Icons::Shutdown()
	{
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


}
