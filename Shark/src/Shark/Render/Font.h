#pragma once

#include "Shark/Render/Texture.h"

#include <filesystem>

namespace Shark {

	class Font
	{
	public:
		static void Initialize();
		static void Shutdown();

		static std::pair<Ref<Texture2D>, Ref<Texture2D>> Test(const std::filesystem::path& fontPath);
	};

}
