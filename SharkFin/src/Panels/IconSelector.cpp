#include "skfpch.h"
#include "IconSelector.h"

#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/UI/UI.h"
#include "Shark/UI/Theme.h"
#include "Shark/UI/EditorResources.h"

#include "Shark/Utils/PlatformUtils.h"

namespace Shark {

	extern std::filesystem::path g_GenericFileIconPath;
	extern std::filesystem::path g_FolderIconPath;
	extern std::filesystem::path g_PNGIconPath;
	extern std::filesystem::path g_JPGIconPath;
	extern std::filesystem::path g_SceneIconPath;
	extern std::filesystem::path g_ScriptIconPath;
	extern std::filesystem::path g_TextureIconPath;
	extern std::filesystem::path g_PlayIconPath;
	extern std::filesystem::path g_StopIconPath;
	extern std::filesystem::path g_PauseIconPath;
	extern std::filesystem::path g_SimulateIconPath;
	extern std::filesystem::path g_StepIconPath;
	extern std::filesystem::path g_ClearIconPath;
	extern std::filesystem::path g_ReloadIconPath;
	extern std::filesystem::path g_AngleLeftIconPath;
	extern std::filesystem::path g_AngleRightIconPath;
	extern std::filesystem::path g_SettingsIconPath;
	extern std::filesystem::path g_SearchIconPath;
	extern std::filesystem::path g_AlphaBackgroundPath;

	namespace utils {

		static void FileDialogControl(std::string_view label, std::filesystem::path& path, Ref<Texture2D> texture = nullptr)
		{
			UI::ControlCustom(label, [&]()
			{
				ImGui::SetNextItemAllowOverlap();
				UI::TextFramed(path.string());

				ImGui::SameLine(0.0f, 0.0f);
				const ImVec2 buttonSize = ImGui::CalcTextSize("...") + ImGui::GetStyle().FramePadding * 2.0f;
				UI::ShiftCursorX(-(buttonSize.x));
				ImGui::InvisibleButton("dialogButton", buttonSize);

				ImDrawList* drawList = ImGui::GetWindowDrawList();
				if (ImGui::IsItemActivated())
					drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::TextBrighter, 0.9f), "...");
				else if (ImGui::IsItemHovered())
					drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::ColorWithMultipliedValue(UI::Colors::Theme::TextBrighter, 1.2f), "...");
				else
					drawList->AddText(ImGui::GetItemRectMin(), UI::Colors::Theme::Text, "...");

				if (ImGui::IsItemActivated())
				{
					 auto result = Platform::OpenFileDialog(L"*.*|*.*|png|*.png", 2, path.parent_path(), true);
					 if (!result.empty())
					 {
						 path = FileSystem::Relative(result).generic_wstring();
						 if (texture)
						 {
							 Buffer& buffer = texture->GetBuffer();
							 TextureSpecification& specification = texture->GetSpecification();
							 buffer = TextureImporter::ToBufferFromFile(path, specification.Format, specification.Width, specification.Height);
							 texture->Invalidate();
						 }
					 }
				}
			});
		}

	}

	IconSelector::IconSelector(const std::string& name)
		: Panel(name)
	{
	}

	IconSelector::~IconSelector()
	{
	}

	void IconSelector::OnImGuiRender(bool& shown)
	{
		if (!ImGui::Begin(m_PanelName.c_str(), &shown))
		{
			ImGui::End();
			return;
		}

		UI::Text("Large", "Selected Icons");

		ImGui::NewLine();
		UI::Text("Medium", "Content Browser");
		UI::BeginControls();
		utils::FileDialogControl("Generic File", g_GenericFileIconPath, EditorResources::FileIcon);
		utils::FileDialogControl("Folder", g_FolderIconPath, EditorResources::FolderIcon);
		utils::FileDialogControl("PNG", g_PNGIconPath, EditorResources::PNGIcon);
		utils::FileDialogControl("JPG", g_JPGIconPath, EditorResources::JPGIcon);
		utils::FileDialogControl("Scene", g_SceneIconPath, EditorResources::SceneIcon);
		utils::FileDialogControl("Script", g_ScriptIconPath, EditorResources::ScriptIcon);
		utils::FileDialogControl("Texture", g_TextureIconPath, EditorResources::TextureIcon);
		UI::EndControls();

		ImGui::NewLine();
		UI::Text("Medium", "Viewport Toolbar");
		UI::BeginControls();
		utils::FileDialogControl("Play", g_PlayIconPath, EditorResources::PlayIcon);
		utils::FileDialogControl("Stop", g_StopIconPath, EditorResources::StopIcon);
		utils::FileDialogControl("Pause", g_PauseIconPath, EditorResources::PauseIcon);
		utils::FileDialogControl("Simulate", g_SimulateIconPath, EditorResources::SimulateIcon);
		utils::FileDialogControl("Step", g_StepIconPath, EditorResources::StepIcon);
		utils::FileDialogControl("File", g_GenericFileIconPath, EditorResources::FileIcon);
		UI::EndControls();

		ImGui::NewLine();
		UI::Text("Medium", "Misc");
		UI::BeginControls();
		utils::FileDialogControl("Clear", g_ClearIconPath, EditorResources::ClearIcon);
		utils::FileDialogControl("Reload", g_ReloadIconPath, EditorResources::ReloadIcon);
		utils::FileDialogControl("Angle Left", g_AngleLeftIconPath, EditorResources::AngleLeftIcon);
		utils::FileDialogControl("Angle Right", g_AngleRightIconPath, EditorResources::AngleRightIcon);
		utils::FileDialogControl("Settings", g_SettingsIconPath, EditorResources::SettingsIcon);
		utils::FileDialogControl("Search", g_SearchIconPath, EditorResources::SearchIcon);
		utils::FileDialogControl("Alpha Background", g_AlphaBackgroundPath, EditorResources::AlphaBackground);
		UI::EndControls();

		ImGui::End();
	}

}
