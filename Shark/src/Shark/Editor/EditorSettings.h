#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/RefCount.h"

namespace Shark {

	class EditorSettings : public RefCount
	{
	public:
		static void Init();
		static void Shutdown();

		static EditorSettings& Get();

	public:
		float ContentBrowserThumbnailSize = 120.0f;
	};

}
