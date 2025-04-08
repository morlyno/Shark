#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class EditorResources
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

		// Viewport Toolbar
		inline static Ref<Texture2D> PlayIcon;
		inline static Ref<Texture2D> StopIcon;
		inline static Ref<Texture2D> PauseIcon;
		inline static Ref<Texture2D> SimulateIcon;
		inline static Ref<Texture2D> StepIcon;

		// Custom Titlebar
		inline static Ref<Texture2D> WindowCloseIcon;
		inline static Ref<Texture2D> WindowMinimizeIcon;
		inline static Ref<Texture2D> WindowMaximizeIcon;
		inline static Ref<Texture2D> WindowRestoreIcon;

		// Misc
		inline static Ref<Texture2D> ClearIcon;
		inline static Ref<Texture2D> ReloadIcon;
		inline static Ref<Texture2D> AngleLeftIcon;
		inline static Ref<Texture2D> AngleRightIcon;
		inline static Ref<Texture2D> SettingsIcon;
		inline static Ref<Texture2D> SearchIcon;
		inline static Ref<Texture2D> ResetIcon;

		inline static Ref<Texture2D> AlphaBackground;

	public:
		static void Init();
		static void Shutdown();
		static void ReloadIcons();

	};

}
