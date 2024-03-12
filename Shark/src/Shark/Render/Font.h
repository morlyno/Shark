#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Texture.h"

#include <filesystem>

namespace Shark {

	struct MSDFData;

	class Font : public Asset
	{
	public:
		Font() = default;
		Font(const std::filesystem::path& fontPath);
		~Font();

		void Load(const std::filesystem::path& fontPath);

		const MSDFData* GetMSDFData() const { return m_MSDFData; }
		Ref<Texture2D> GetFontAtlas() const { return m_FontAtlas; }

	public:
		static AssetType GetStaticType() { return AssetType::Font; }
		virtual AssetType GetAssetType() const { return GetStaticType(); }

		static Ref<Font> Create() { return Ref<Font>::Create(); }
	private:
		void Init(const std::filesystem::path& fontPath);

	private:
		MSDFData* m_MSDFData = nullptr;
		Ref<Texture2D> m_FontAtlas;
	};

}
