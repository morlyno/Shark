#include "AssetsPanel.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/Widgets.h"
#include "Shark/Utils/String.h"

namespace Shark {

	AssetsPanel::AssetsPanel(const std::string& panelName)
		: Panel(panelName)
	{
		m_Pattern.EnabelAutoCaseSensitive();
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

			UI::Widgets::Search(m_Pattern);

			ImGui::SameLine();
			if (ImGui::Button("+", ImVec2(settingsButtonSize, settingsButtonSize)))
				ImGui::OpenPopup("Settings");
			if (ImGui::BeginPopup("Settings"))
			{
				ImGui::Checkbox("Edit", &m_Edit);
				ImGui::Separator();

				UI::BeginControlsGrid();
				
				for (auto& [type, enabled] : m_EnabledTypes)
					UI::Control(magic_enum::enum_name(type), enabled);

				UI::EndControlsGrid();

				if (ImGui::Selectable("Toggle"))
				{
					for (auto& [type, enabled] : m_EnabledTypes)
						enabled = !enabled;
				}

				ImGui::EndPopup();
			}
		}

		const auto drawEntry = [this](const AssetMetaData& metadata)
		{
			if (!IsAssetTypeEnabled(metadata.Type))
				return;

			if (m_Pattern)
			{
				if (!m_Pattern.PassesFilter(fmt::to_string(metadata.Handle)) &&
					!m_Pattern.PassesFilter(magic_enum::enum_name(metadata.Type)) &&
					!m_Pattern.PassesFilter(metadata.FilePath.string()))
					return;
			}

			UI::BeginControlsGrid();
			UI::Control("Handle", fmt::to_string(metadata.Handle));
			UI::Control("FilePath", metadata.FilePath.string());
			UI::Control("Type", std::string(magic_enum::enum_name(metadata.Type)));
			UI::EndControlsGrid();

			ImGui::Separator();
		};

		if (ImGui::TreeNodeEx("Imported Assets", UI::DefaultThinHeaderFlags | ImGuiTreeNodeFlags_DefaultOpen))
		{
			const auto& registry = Project::GetEditorAssetManager()->GetAssetRegistry();
			for (auto& [handle, metadata] : registry)
			{
				drawEntry(metadata);
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("Loaded Assets", UI::DefaultThinHeaderFlags))
		{
			const auto& loadedAssets = Project::GetEditorAssetManager()->GetLoadedAssets();
			for (const auto& [handle, asset] : loadedAssets)
			{
				const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(asset);
				drawEntry(metadata);
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
