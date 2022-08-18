#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Texture.h"

namespace Shark {

	class Icons
	{
	public:
		static Ref<Image2D> FileIcon;
		static Ref<Image2D> FolderIcon;
		static Ref<Image2D> PNGIcon;
		static Ref<Image2D> SceneIcon;
		static Ref<Image2D> ScriptIcon;
		static Ref<Image2D> TextureIcon;

		static Ref<Image2D> InfoIcon;
		static Ref<Image2D> WarnIcon;
		static Ref<Image2D> ErrorIcon;

		static Ref<Image2D> PlayIcon;
		static Ref<Image2D> StopIcon;
		static Ref<Image2D> PauseIcon;
		static Ref<Image2D> SimulateIcon;
		static Ref<Image2D> StepIcon;

	public:
		static void Init();
		static void Shutdown();

	};

}
