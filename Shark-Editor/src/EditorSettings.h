#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"

namespace Shark {

	class EditorSettings
	{
	public:
		static void Init();
		static void Shutdown();

		static EditorSettings& Get();

	public:
		struct ContentBrowserSettings
		{
			float ThumbnailSize = 120.0f;
			bool GenerateThumbnails = true;
		};
		ContentBrowserSettings ContentBrowser;

		bool Prefab_AutoGroupRootEntities = true;
	};

}
