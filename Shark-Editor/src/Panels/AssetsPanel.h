#pragma once

#include "Shark/Asset/AssetTypes.h"
#include "Shark/UI/TextFilter.h"
#include "Panel.h"

namespace Shark {

	class AssetsPanel : public Panel
	{
	public:
		AssetsPanel();
		virtual ~AssetsPanel();

		virtual void OnImGuiRender(bool& shown) override;

		static const char* GetStaticID() { return "AssetsPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }

	private:
		bool IsAssetTypeEnabled(AssetType assetType);
	private:
		// TODO(moro): Add AssetManager or Project member

		UI::TextFilter m_Pattern;
		bool m_SearchHasUppercase = false;
		bool m_CaseSensitive = false;
		bool m_Edit = false;

		//struct AssetTypeFlag
		//{
		//	enum Type : uint16_t
		//	{
		//		None = 0,
		//		Scene = BIT(0),
		//		Texture = BIT(1),
		//		TextureSource = BIT(2),
		//		ScriptFile = BIT(3),
		//		Font = BIT(4),
		//
		//		All = Scene | Texture | TextureSource | ScriptFile | Font
		//	};
		//	using Flags = std::underlying_type_t<Type>;
		//};

		std::map<AssetType, bool> m_EnabledTypes;

		//AssetTypeFlag::Flags m_EnabledTypes = AssetTypeFlag::All;
	};

}
