#pragma once

#include "Shark/Render/Texture.h"

#include <filesystem>

namespace Shark {

	struct MSDFData;

	class Font : public RefCount
	{
	public:
		Font(const std::filesystem::path& fontPath);
		~Font();

		const MSDFData* GetMSDFData() const { return m_MSDFData; }
		Ref<Texture2D> GetFontAtlas() const { return m_FontAtlas; }

	private:
		void Init(const std::filesystem::path& fontPath);

	private:
		MSDFData* m_MSDFData = nullptr;
		Ref<Texture2D> m_FontAtlas;
	};

}
