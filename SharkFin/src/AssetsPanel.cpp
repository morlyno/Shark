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

		static const char* GetDragDropType(const std::filesystem::path& file)
		{
			auto&& extension = file.extension();
			if (extension == ".shark")
				return DragDropType::Scean;
			if (extension == ".png")
				return DragDropType::Texture;
			return DragDropType::Typeless;
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
		if (!m_ShowPanel)
			return;

		if (!ImGui::Begin("Assets", &m_ShowPanel))
		{
			ImGui::End();
			return;
		}

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 0.0f, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.5f, 0.5f, 0.5f, 0.1f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.5f, 0.5f, 0.5f, 0.3f });

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		window->DC.CursorPos.y -= ImGui::GetStyle().FramePadding.y;

		// Navigation Buttons
		DrawNavigationButtons();

		// File Search
		ImGui::SameLine();
		DrawFilterInput();

		// Current Path
		ImGui::SameLine();
		DrawCurrentPath();

		ImGui::Separator();

		if (ImGui::BeginTable("AssetsPanel", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableNextRow();

			// Directorys
			ImGui::TableSetColumnIndex(0);
			constexpr ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_DefaultOpen;
			const bool opened = ImGui::TreeNodeEx("assets", flags);

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
		m_CurrentPath = path;

		if (m_DirHistoryIndex != m_DirectoryHistory.size() - 1)
		{
			auto& d = m_DirectoryHistory;
			d.erase(d.begin() + m_DirHistoryIndex + 1, d.end());
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

		for (DirectoryEntry&& entry : std::filesystem::directory_iterator(directory))
		{
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
		std::filesystem::path relativePath = std::filesystem::relative(m_CurrentPath);
		const auto begin = relativePath.begin();
		const auto end = relativePath.end();
		for (auto pathElem = begin; pathElem != end; ++pathElem)
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

			ImGui::AlignTextToFramePadding();
			if (isHoverd)
				ImGui::Text(label.c_str());
			else
				ImGui::TextDisabled(label.c_str());

			if (pathElem != --relativePath.end())
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled("\\\\");
				ImGui::SameLine(0.0f, 0.0f);
			}

			if (isClicked)
				OnDirectoryClicked(Utility::CreatePathFormIterator(begin, std::next(pathElem)));
		}
	}

	void AssetsPanel::DrawDirectoryContent()
	{
		const float Padding = 8.0f;
		const int Collums = ImGui::GetContentRegionAvailWidth() / (m_ImageSize.x + Padding);

		if (ImGui::BeginTable("FilesAndFolders", std::clamp(Collums, 1, 64)))
		{
			for (DirectoryEntry entry : std::filesystem::directory_iterator(m_CurrentPath))
			{
				ImGui::TableNextColumn();
				const bool isDirectory = entry.Entry.is_directory();
				const RenderID imageID = isDirectory ? m_DirectoryImage->GetRenderID() : m_FileImage->GetRenderID();
				DrawDirectoryEntry(imageID, entry);
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
			for (DirectoryEntry entry : std::filesystem::recursive_directory_iterator(Utils::AssetsDirectory()))
			{
				const bool passedFilter = CheckFileOnFilter(entry.FileName);

				if (passedFilter)
				{
					ImGui::TableNextColumn();
					const bool isDirectory = entry.Entry.is_directory();
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
		const bool hovered = ImGui::IsItemHovered();

		if (entry.Entry.is_regular_file())
		{
			if (ImGui::BeginDragDropSource())
			{
				SK_CORE_ASSERT(sizeof(*entry.PathString.data()) == 1);
				SK_CORE_ASSERT(!entry.PathString.empty());
				ImGui::SetDragDropPayload(Utils::GetDragDropType(entry.Path), entry.PathString.c_str(), entry.PathString.length());
				ImGui::EndDragDropSource();
			}
		}

		if (pressed)
		{
			if (entry.Entry.is_directory())
				OnDirectoryClicked(entry.Path);
			else if (entry.Entry.is_regular_file())
			{
				OnFileClicked(entry.Path);
			}
			else
				SK_CORE_ASSERT(false);
		}

		if (hovered)
			ImGui::SetTooltip(entry.PathString.c_str());


		ImGui::TextWrapped(entry.FileNameShort.c_str());

		ImGui::PopID();
	}

}