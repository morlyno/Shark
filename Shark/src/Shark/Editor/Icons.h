#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Icons
	{
	public:
		static Ref<Texture2D> SettingsIcon;

		static Ref<Texture2D> FileIcon;
		static Ref<Texture2D> FolderIcon;
		static Ref<Texture2D> PNGIcon;
		static Ref<Texture2D> SceneIcon;
		static Ref<Texture2D> ScriptIcon;
		static Ref<Texture2D> TextureIcon;

		static Ref<Texture2D> InfoIcon;
		static Ref<Texture2D> WarnIcon;
		static Ref<Texture2D> ErrorIcon;

		static Ref<Texture2D> PlayIcon;
		static Ref<Texture2D> StopIcon;
		static Ref<Texture2D> PauseIcon;
		static Ref<Texture2D> SimulateIcon;
		static Ref<Texture2D> StepIcon;

	public:
		static void Init();
		static void InitWithDummyImages();
		static void Shutdown();
		static void Reload();

	};

}
