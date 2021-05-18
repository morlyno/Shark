#include "AssetsPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Shark {

	namespace Utils {

		static std::filesystem::path AssetsDirectory()
		{
			return std::filesystem::current_path() / "assets\\";
		}

	}

	AssetsPanel::AssetsPanel()
	{
		m_CurrentPath = Utils::AssetsDirectory();
		m_DirectoryHistory.emplace_back(m_CurrentPath); 
		m_DirHistoryIndex;

		m_FileImage = Texture2D::Create({}, "assets/Textures/file.png");
		m_DirectoryImage = Texture2D::Create({}, "assets/Textures/folder_open.png");

		m_FilterBuffer.reserve(16);
	}

	AssetsPanel::~AssetsPanel()
	{
	}

	void AssetsPanel::OnImGuiRender()
	{
		ImGui::Begin("Assets");

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.5f, 0.5f, 0.5f, 0.1f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.5f, 0.5f, 0.5f, 0.3f });

		// Navigation Buttons
		DrawNavigationButtons();

		// File Search
		ImGui::SameLine();
		DrawFilterInput();

		// Current Path
		ImGui::SameLine();
		DrawCurrentPath();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos.y += 8.0f;

		ImGui::Separator();

		if (ImGui::BeginTable("AssetsPanel", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();

			// Directorys
			ImGui::TableSetColumnIndex(0);
			const bool opened = ImGui::TreeNodeEx("assets", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow);

			if (ImGui::IsItemClicked())
				OnDirectoryClicked(Utils::AssetsDirectory());

			if (opened)
			{
				DrawDirectory(Utils::AssetsDirectory());
				ImGui::TreePop();
			}

			// Directory
			ImGui::TableSetColumnIndex(1);
			if (m_ShowFiltered)
				DrawAssetsFiltered();
			else
				DrawDirectoryContent();

			ImGui::EndTable();
		}

		if (m_WantResetFilter)
			ResetFilter();

		ImGui::PopStyleColor(3);

		ImGui::End();
	}

	void AssetsPanel::OnDirectoryClicked(const std::filesystem::path& path)
	{
		m_WantResetFilter = true;
		SK_CORE_TRACE("Folder Clicked: {0}", path);
		m_CurrentPath = path;

		if (m_DirHistoryIndex != m_DirectoryHistory.size() - 1)
		{
			SK_CORE_TRACE("------- History overriden -------");
			for (auto& d : m_DirectoryHistory)
				SK_CORE_TRACE("Old: {0}", d);
			auto& d = m_DirectoryHistory;
			d.erase(d.begin() + m_DirHistoryIndex + 1, d.end());
			for (auto& d : m_DirectoryHistory)
				SK_CORE_TRACE("New: {0}", d);
			SK_CORE_TRACE("New: {0}", path);

		}

		m_DirHistoryIndex = m_DirectoryHistory.size();
		m_DirectoryHistory.emplace_back(path);
	}

	void AssetsPanel::OnFileClicked(const std::filesystem::path& path)
	{
		m_WantResetFilter = true;
		SK_CORE_TRACE("File Clicked: {0}", path);
	}

	void AssetsPanel::DrawDirectory(const std::filesystem::path& directory)
	{
		constexpr ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

		for (auto&& e : std::filesystem::directory_iterator(directory))
		{
			DirectoryEntry entry{ e };

			if (entry.Entry.is_directory())
			{
				const bool isOpen = ImGui::TreeNodeEx(entry.FileName.c_str(), flags);
				const bool isClicked = ImGui::IsItemClicked();
				
				if (isClicked)
					OnDirectoryClicked(entry.Path);

				if (isOpen)
				{
					DrawDirectory(entry.Path);
					ImGui::TreePop();
				}
			}
			else
			{
				ImGui::TreeNodeEx(entry.FileNameShort.c_str(), flags | ImGuiTreeNodeFlags_Bullet);
			}
		}
	}

	void AssetsPanel::DrawNavigationButtons()
	{
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });

		if (ImGui::ArrowButton("DirectoryBack", ImGuiDir_Left))
			if (m_DirHistoryIndex > 0)
				m_CurrentPath = m_DirectoryHistory[--m_DirHistoryIndex];

		ImGui::SameLine();
		if (ImGui::ArrowButton("DirectoryForward", ImGuiDir_Right))
			if (m_DirHistoryIndex < m_DirectoryHistory.size() - 1)
				m_CurrentPath = m_DirectoryHistory[++m_DirHistoryIndex];

		ImGui::PopStyleColor();
	}

	void AssetsPanel::DrawCurrentPath()
	{
		// TODO: Create owne relative funtion (this bullshit uses aroung 50 allocations!!!!)
		std::filesystem::path relativePath = std::filesystem::relative(m_CurrentPath);
		for (auto&& pathElem = relativePath.begin(); pathElem != relativePath.end(); ++pathElem)
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			const ImVec2 prevCursor = window->DC.CursorPos;

			std::string label = pathElem->string();
			const ImVec2 label_size = ImGui::CalcTextSize(label.c_str(), NULL, true);

			auto& style = ImGui::GetStyle();
			ImVec2 size = ImGui::CalcItemSize({ 0, 0 }, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

			const bool isClicked = ImGui::InvisibleButton(label.c_str(), size);
			const bool isHoverd = ImGui::IsItemHovered();

			window->DC.CursorPos = prevCursor;
			float fontSize = ImGui::GetFontSize();
			float cursorOffsetY = (size.y - fontSize) * 0.5f;
			window->DC.CursorPos.y += cursorOffsetY;

			if (isHoverd)
				ImGui::Text(label.c_str());
			else
				ImGui::TextDisabled(label.c_str());

			window->DC.CursorPosPrevLine.y -= cursorOffsetY;

			if (pathElem != --relativePath.end())
			{
				ImGui::SameLine(0.0f, 0.0f);
				window->DC.CursorPos.y += cursorOffsetY;
				ImGui::TextDisabled("\\\\");
				window->DC.CursorPosPrevLine.y -= cursorOffsetY;
				ImGui::SameLine(0.0f, 0.0f);
			}

			if (isClicked)
				OnDirectoryClicked(Utility::CreatePathFormIterator(relativePath.begin(), std::next(pathElem)));
		}
	}

	void AssetsPanel::DrawDirectoryContent()
	{
		const float Padding = 8.0f;
		const int Collums = ImGui::GetContentRegionAvailWidth() / (m_ImageSize.x + Padding);

		if (ImGui::BeginTable("FilesAndFolders", Collums))
		{
			for (auto&& entry : std::filesystem::directory_iterator(m_CurrentPath))
			{
				ImGui::TableNextColumn();
				DirectoryEntry temp{ entry };
				const bool isDirectory = entry.is_directory();
				const RenderID imageID = isDirectory ? m_DirectoryImage->GetRenderID() : m_FileImage->GetRenderID();
				DrawDirectoryEntry(imageID, temp);
			}

			ImGui::EndTable();
		}
	}

	void AssetsPanel::DrawFilterInput()
	{
		ImGui::SetNextItemWidth(150);
		if (ImGui::InputText("##FilterInput", &m_FilterBuffer))
			m_FilterAsLower = Utility::ToLower(m_FilterBuffer);

		m_ShowFiltered = !m_FilterBuffer.empty();
	}

	void AssetsPanel::DrawAssetsFiltered()
	{
		const float Padding = 8.0f;
		const int Collums = ImGui::GetContentRegionAvailWidth() / (m_ImageSize.x + Padding);

		if (ImGui::BeginTable("FilesAndFolders", Collums))
		{
			for (auto&& e : std::filesystem::recursive_directory_iterator(Utils::AssetsDirectory()))
			{
				DirectoryEntry entry{ e };
				const bool passedFilter = CheckFileOnFilter(entry.FileName);

				if (passedFilter)
				{
					ImGui::TableNextColumn();
					const bool isDirectory = e.is_directory();
					const RenderID imageID = isDirectory ? m_DirectoryImage->GetRenderID() : m_FileImage->GetRenderID();

					DrawDirectoryEntry(imageID, entry);
				}
			}

			ImGui::EndTable();
		}
	}

	bool AssetsPanel::CheckFileOnFilter(const std::string& str)
	{
		auto&& lsrc = Utility::ToLower(str);
		auto&& lfilter = m_FilterAsLower;
		return lsrc.find(lfilter) != std::string::npos;
	}

	void AssetsPanel::ResetFilter()
	{
		m_WantResetFilter = false;
		m_ShowFiltered = false;
		m_FilterBuffer.clear();
		m_FilterAsLower.clear();
	}

	void AssetsPanel::DrawDirectoryEntry(RenderID imageID, const DirectoryEntry& entry)
	{
		ImGui::PushID(entry.FileName.c_str());

		const bool pressed = ImGui::ImageButton(imageID, m_ImageSize);

		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(entry.PathString.c_str());

		ImGui::TextWrapped(entry.FileNameShort.c_str());

		if (pressed)
		{
			if (entry.Entry.is_directory())
				OnDirectoryClicked(entry.Path);
			else
				OnFileClicked(entry.Path);
		}

		ImGui::PopID();
	}

}