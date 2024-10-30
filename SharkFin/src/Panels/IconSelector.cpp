#include "skfpch.h"
#include "IconSelector.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Serialization/Import/TextureImporter.h"

#include "Shark/UI/UICore.h"
#include "Shark/UI/Widgets.h"
#include "Shark/UI/Controls.h"
#include "Shark/UI/EditorResources.h"

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
				if (UI::Widgets::InputFile(UI::DialogType::Open, path, "*.*|*.*|png|*.png", path.parent_path()))
				{
					path = FileSystem::Relative(path).generic_wstring();
					if (texture)
					{
						Buffer& buffer = texture->GetBuffer();
						TextureSpecification& specification = texture->GetSpecification();
						buffer = TextureImporter::ToBufferFromFile(path, specification.Format, specification.Width, specification.Height);
						texture->Invalidate();
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
		UI::BeginControlsGrid();
		utils::FileDialogControl("Generic File", g_GenericFileIconPath, EditorResources::FileIcon);
		utils::FileDialogControl("Folder", g_FolderIconPath, EditorResources::FolderIcon);
		utils::FileDialogControl("PNG", g_PNGIconPath, EditorResources::PNGIcon);
		utils::FileDialogControl("JPG", g_JPGIconPath, EditorResources::JPGIcon);
		utils::FileDialogControl("Scene", g_SceneIconPath, EditorResources::SceneIcon);
		utils::FileDialogControl("Script", g_ScriptIconPath, EditorResources::ScriptIcon);
		utils::FileDialogControl("Texture", g_TextureIconPath, EditorResources::TextureIcon);
		UI::BeginControlsGrid();

		ImGui::NewLine();
		UI::Text("Medium", "Viewport Toolbar");
		UI::BeginControlsGrid();
		utils::FileDialogControl("Play", g_PlayIconPath, EditorResources::PlayIcon);
		utils::FileDialogControl("Stop", g_StopIconPath, EditorResources::StopIcon);
		utils::FileDialogControl("Pause", g_PauseIconPath, EditorResources::PauseIcon);
		utils::FileDialogControl("Simulate", g_SimulateIconPath, EditorResources::SimulateIcon);
		utils::FileDialogControl("Step", g_StepIconPath, EditorResources::StepIcon);
		utils::FileDialogControl("File", g_GenericFileIconPath, EditorResources::FileIcon);
		UI::EndControlsGrid();

		ImGui::NewLine();
		UI::Text("Medium", "Misc");
		UI::BeginControlsGrid();
		utils::FileDialogControl("Clear", g_ClearIconPath, EditorResources::ClearIcon);
		utils::FileDialogControl("Reload", g_ReloadIconPath, EditorResources::ReloadIcon);
		utils::FileDialogControl("Angle Left", g_AngleLeftIconPath, EditorResources::AngleLeftIcon);
		utils::FileDialogControl("Angle Right", g_AngleRightIconPath, EditorResources::AngleRightIcon);
		utils::FileDialogControl("Settings", g_SettingsIconPath, EditorResources::SettingsIcon);
		utils::FileDialogControl("Search", g_SearchIconPath, EditorResources::SearchIcon);
		utils::FileDialogControl("Alpha Background", g_AlphaBackgroundPath, EditorResources::AlphaBackground);
		UI::EndControlsGrid();

		ImGui::End();
	}

}
