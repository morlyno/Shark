#pragma once

#include "Shark/Asset/AssetTypes.h"
#include "Shark/Editor/Panel.h"

namespace Shark {

	class AssetsPanel : public Panel
	{
	public:
		AssetsPanel(const char* panelName);
		virtual ~AssetsPanel();

		virtual void OnImGuiRender(bool& shown) override;
	private:
		bool IsAssetTypeEnabled(AssetType assetType);
	private:
		char m_SearchBuffer[260];
		bool m_SearchHasUppercase = false;
		bool m_CaseSensitive = false;

		struct AssetTypeFlag
		{
			enum Type : uint16_t
			{
				None = 0,
				Scene = BIT(0),
				Texture = BIT(1),
				TextureSource = BIT(2),
				ScriptFile = BIT(3),

				All = Scene | Texture | TextureSource | ScriptFile
			};
			using Flags = std::underlying_type_t<Type>;
		};

		AssetTypeFlag::Flags m_EnabledTypes = AssetTypeFlag::All;
	};

}
