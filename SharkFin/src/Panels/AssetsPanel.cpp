#include "skfpch.h"
#include "AssetsPanel.h"

#include "Shark/UI/UI.h"
#include "Shark/Utils/String.h"

namespace Shark {

	AssetsPanel::AssetsPanel(const std::string& panelName)
		: Panel(panelName)
	{
		memset(m_SearchBuffer, 0, sizeof(m_SearchBuffer));
	}

	AssetsPanel::~AssetsPanel()
	{

	}

	void AssetsPanel::OnImGuiRender(bool& shown)
	{
		if (!shown)
			return;

		const ImGuiStyle& style = ImGui::GetStyle();
		UI::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, style.WindowPadding * 0.5f);
		ImGui::Begin(m_PanelName.c_str(), &shown, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		{
			UI::ScopedStyle itemSpacing(ImGuiStyleVar_ItemSpacing, { style.ItemSpacing.x * 0.5f, style.ItemSpacing.y });
			const float settingsButtonSize = ImGui::GetFrameHeight();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (settingsButtonSize + style.ItemSpacing.x));

			if (UI::Search(UI::GenerateID(), m_SearchBuffer, 260))
			{
				m_SearchHasUppercase = false;
				for (auto& c : m_SearchBuffer)
				{
					if (isupper(c))
					{
						m_SearchHasUppercase = true;
						break;
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(settingsButtonSize, settingsButtonSize)))
				ImGui::OpenPopup("Settings");
			if (ImGui::BeginPopup("Settings"))
			{
				ImGui::Checkbox("Edit", &m_Edit);
				ImGui::Separator();

				UI::BeginControls();
				
				for (auto& [type, enabled] : m_EnabledTypes)
					UI::Control(ToString(type), enabled);

				UI::EndControls();

				if (ImGui::Selectable("Toggle"))
				{
					for (auto& [type, enabled] : m_EnabledTypes)
						enabled = !enabled;
				}

				ImGui::EndPopup();
			}
		}

		std::string_view searchBufferView = m_SearchBuffer;

		if (ImGui::TreeNodeEx("Imported Assets", UI::DefaultThinHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto& registry = Project::GetActiveEditorAssetManager()->GetAssetRegistry();
			for (auto& [handle, metadata] : registry)
			{
				if (!IsAssetTypeEnabled(metadata.Type))
					continue;

				if (!searchBufferView.empty())
				{
					std::string handleStr = fmt::to_string(metadata.Handle);
					std::string typeStr = ToString(metadata.Type);
					std::string filePathStr = metadata.FilePath.string();

					if (!m_SearchHasUppercase)
					{
						String::ToLower(handleStr);
						String::ToLower(typeStr);
						String::ToLower(filePathStr);
					}
					bool matchFound = false;

					matchFound |= handleStr.find(searchBufferView) != std::string::npos;
					matchFound |= filePathStr.find(searchBufferView) != std::string::npos;
					matchFound |= typeStr.find(searchBufferView) != std::string::npos;

					if (!matchFound)
						continue;
				}

				UI::BeginControlsGrid();
				UI::Property("Handle", metadata.Handle);
				UI::Property("FilePath", metadata.FilePath);
				UI::Property("Type", magic_enum::enum_name(metadata.Type));
				UI::EndControls();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Loaded Assets", UI::DefaultThinHeaderFlags))
		{
			for (const auto& [handle, asset] : Project::GetActiveEditorAssetManager()->GetLoadedAssets())
			{
				const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(asset);
				if (!IsAssetTypeEnabled(metadata.Type))
					continue;

				if (!searchBufferView.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					std::string typeStr = ToString(metadata.Type);
					std::string filePathStr = metadata.FilePath.string();

					if (!m_SearchHasUppercase)
					{
						String::ToLower(handleStrHex);
						String::ToLower(typeStr);
						String::ToLower(filePathStr);
					}

					bool matchFound = false;

					matchFound |= handleStrHex.find(searchBufferView) != std::string::npos;
					matchFound |= filePathStr.find(searchBufferView) != std::string::npos;
					matchFound |= typeStr.find(searchBufferView) != std::string::npos;

					if (!matchFound)
						continue;
				}


				UI::BeginControlsGrid();

				UI::Property("Handle", metadata.Handle);
				UI::Property("FilePath", metadata.FilePath);
				UI::Property("Type", ToString(metadata.Type));
				//UI::Property("Memory", metadata.IsMemoryAsset);

				UI::EndControls();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}

	bool AssetsPanel::IsAssetTypeEnabled(AssetType assetType)
	{
		if (!m_EnabledTypes.contains(assetType))
			m_EnabledTypes[assetType] = true;

		return m_EnabledTypes.at(assetType);

		//switch (assetType)
		//{
		//	case AssetType::None:          return false;
		//	case AssetType::Scene:         return m_EnabledTypes & AssetTypeFlag::Scene;
		//	case AssetType::Texture:       return m_EnabledTypes & AssetTypeFlag::Texture;
		//	case AssetType::TextureSource: return m_EnabledTypes & AssetTypeFlag::TextureSource;
		//	case AssetType::ScriptFile:    return m_EnabledTypes & AssetTypeFlag::ScriptFile;
		//	case AssetType::Font:          return m_EnabledTypes & AssetTypeFlag::Font;
		//}
		//
		//SK_CORE_ASSERT(false, "Unkown AssetType");
		//return false;
	}

}
