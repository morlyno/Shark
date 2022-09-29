#include "skfpch.h"
#include "AssetsPanel.h"

#include "Shark/Asset/ResourceManager.h"
#include "Shark/UI/UI.h"
#include "Shark/Utils/String.h"

namespace Shark {

	AssetsPanel::AssetsPanel(const char* panelName)
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
		ImGui::Begin(PanelName, &shown, ImGuiWindowFlags_AlwaysVerticalScrollbar);

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
				UI::BeginControls();
				UI::ControlFlags("All", m_EnabledTypes, AssetTypeFlag::All);
				UI::ControlFlags("Scene", m_EnabledTypes, AssetTypeFlag::Scene);
				UI::ControlFlags("Texture", m_EnabledTypes, AssetTypeFlag::Texture);
				UI::ControlFlags("TextureSource", m_EnabledTypes, AssetTypeFlag::TextureSource);
				UI::ControlFlags("ScriptFile", m_EnabledTypes, AssetTypeFlag::ScriptFile);
				UI::EndControls();
				ImGui::EndPopup();
			}
		}

		std::string_view searchBufferView = m_SearchBuffer;

		if (ImGui::TreeNodeEx("Imported Assets", UI::TreeNodeSeperatorFlags | ImGuiTreeNodeFlags_DefaultOpen))
		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);

			for (const auto& [handle, metadata] : ResourceManager::GetAssetRegistry())
			{
				if (!IsAssetTypeEnabled(metadata.Type))
					continue;

				if (!searchBufferView.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					std::string typeStr = AssetTypeToString(metadata.Type);
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

				const UI::TextFlags textFlags = UI::TextFlag::Selectable | UI::TextFlag::Aligned;
				char buffer[sizeof("0x0123456789ABCDEF")];
				sprintf_s(buffer, "0x%llx", (uint64_t)metadata.Handle);
				UI::Property("Handle", buffer, textFlags);
				UI::Property("FilePath", metadata.FilePath, textFlags);
				UI::Property("Type", AssetTypeToString(metadata.Type), textFlags);

				UI::EndControls();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Loaded Assets", UI::TreeNodeSeperatorFlags))
		{
			UI::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);

			for (const auto& [handle, asset] : ResourceManager::GetLoadedAssets())
			{
				const auto& metadata = ResourceManager::GetMetaData(asset);
				if (!IsAssetTypeEnabled(metadata.Type))
					continue;

				if (!searchBufferView.empty())
				{
					std::string handleStrHex = fmt::format("{:x}", metadata.Handle);
					std::string typeStr = AssetTypeToString(metadata.Type);
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

				const UI::TextFlags textFlags = UI::TextFlag::Selectable | UI::TextFlag::Aligned;
				char buffer[sizeof("0x0123456789ABCDEF")];
				sprintf_s(buffer, "0x%llx", (uint64_t)metadata.Handle);
				UI::Property("Handle", buffer, textFlags);
				UI::Property("FilePath", metadata.FilePath, textFlags);
				UI::Property("Type", AssetTypeToString(metadata.Type), textFlags);
				bool isMemoryAsset = metadata.IsMemoryAsset;
				UI::Control("Memory", isMemoryAsset);

				UI::EndControls();

				ImGui::Separator();
			}

			ImGui::TreePop();
		}

		ImGui::End();
	}

	bool AssetsPanel::IsAssetTypeEnabled(AssetType assetType)
	{
		switch (assetType)
		{
			case AssetType::None:          return false;
			case AssetType::Scene:         return m_EnabledTypes & AssetTypeFlag::Scene;
			case AssetType::Texture:       return m_EnabledTypes & AssetTypeFlag::Texture;
			case AssetType::TextureSource: return m_EnabledTypes & AssetTypeFlag::TextureSource;
			case AssetType::ScriptFile:    return m_EnabledTypes & AssetTypeFlag::ScriptFile;
		}

		SK_CORE_ASSERT(false, "Unkown AssetType");
		return false;
	}

}
