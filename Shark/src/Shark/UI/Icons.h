#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Icons
	{
	public:

		// Content Browser
		inline static Ref<Texture2D> FileIcon;
		inline static Ref<Texture2D> FolderIcon;
		inline static Ref<Texture2D> PNGIcon;
		inline static Ref<Texture2D> JPGIcon;
		inline static Ref<Texture2D> SceneIcon;
		inline static Ref<Texture2D> ScriptIcon;
		inline static Ref<Texture2D> TextureIcon;

		// Scene Play
		inline static Ref<Texture2D> PlayIcon;
		inline static Ref<Texture2D> StopIcon;
		inline static Ref<Texture2D> PauseIcon;
		inline static Ref<Texture2D> SimulateIcon;
		inline static Ref<Texture2D> StepIcon;

		inline static Ref<Texture2D> ClearIcon;
		inline static Ref<Texture2D> ReloadIcon;
		inline static Ref<Texture2D> AngleLeftIcon;
		inline static Ref<Texture2D> AngleRightIcon;
		inline static Ref<Texture2D> SettingsIcon;

		// Custom Titlebar
		inline static Ref<Texture2D> WindowCloseIcon;
		inline static Ref<Texture2D> WindowMinimizeIcon;
		inline static Ref<Texture2D> WindowMaximizeIcon;
		inline static Ref<Texture2D> WindowRestoreIcon;

	public:
		static void Init();
		static void Shutdown();
		static void Reload();

	};

}
