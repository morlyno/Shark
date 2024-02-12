#pragma once

#include "Shark.h"
#include "Shark/Core/Project.h"
#include "Shark/Event/Event.h"
#include "Panel.h"

#include "Panels/ContentBrowser/ContentBrowserItem.h"

#include <future>

#undef CreateDirectory

namespace Shark {

	class CBItemList
	{
	public:
		std::vector<Ref<ContentBrowserItem>> Items;

		std::vector<Ref<ContentBrowserItem>>::iterator begin() { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::iterator end() { return Items.end(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator begin() const { return Items.begin(); }
		std::vector<Ref<ContentBrowserItem>>::const_iterator end() const { return Items.end(); }

		void Clear()
		{
			Items.clear();
		}

		void Sort()
		{
			std::sort(Items.begin(), Items.end(), [](auto lhs, auto rhs) -> bool
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetPath() < rhs->GetPath();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetPath() < rhs->GetPath();
			});
		}

		void Add(Ref<ContentBrowserItem> item)
		{
			const auto it = std::ranges::lower_bound(Items, item, [](auto lhs, auto rhs)
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetPath() < rhs->GetPath();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetPath() < rhs->GetPath();
			});
			Items.insert(it, item);
		}

		bool Contains(Ref<ContentBrowserItem> item)
		{
			return std::ranges::find(Items, item) != Items.end();
		}

		void Remove(Ref<ContentBrowserItem> item)
		{
			const auto it = std::ranges::find(Items, item);
			if (it != Items.end())
			{
				Items.erase(it);
			}
		}

		void MergeWith(const CBItemList& other)
		{
			std::vector<Ref<ContentBrowserItem>> mergesItems;
			mergesItems.reserve(Items.size() + other.Items.size());
			std::ranges::merge(Items, other, std::back_insert_iterator(mergesItems), [](auto lhs, auto rhs)
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetPath() < rhs->GetPath();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetPath() < rhs->GetPath();
			});
			std::swap(Items, mergesItems);
		}

	};

	class CBHistory
	{
	public:
		void Reset(Ref<DirectoryInfo> base)
		{
			m_History.clear();
			m_Index = -1;
			Add(base);
		}

		Ref<DirectoryInfo> Current() const
		{
			SK_CORE_VERIFY(m_Index < m_History.size());
			return m_History[m_Index];
		}

		bool CanMoveBack() const
		{
			return m_Index > 0;
		}

		bool CanMoveForward() const
		{
			return (m_Index + 1) < m_History.size();
		}

		Ref<DirectoryInfo> MoveBack()
		{
			if (m_Index > 0)
			{
				m_Index--;
				return m_History[m_Index];
			}

			return nullptr;
		}

		Ref<DirectoryInfo> MoveForward()
		{
			if ((m_Index + 1) < m_History.size())
			{
				m_Index++;
				return m_History[m_Index];
			}

			return nullptr;
		}

		void Add(Ref<DirectoryInfo> directory)
		{
			if (!directory)
				return;

			m_Index++;
			const auto location = m_History.begin() + m_Index;
			auto inserted = m_History.insert(m_History.begin() + m_Index, directory);
			m_History.erase(inserted + 1, m_History.end());
		}

	private:
		std::vector<Ref<DirectoryInfo>> m_History;
		uint32_t m_Index = -1;
	};

	class CBFilter
	{
	public:
		CBFilter(const std::string& filter, bool caseSensitive)
			: m_CaseSensitive(caseSensitive)
		{
			m_Filter = caseSensitive ? filter : String::ToLowerCopy(filter);
		}

		bool Filter(const std::string& str) const
		{
			if (m_CaseSensitive)
				return str.find(m_Filter) != std::string::npos;

			std::string lowerStr = String::ToLowerCopy(str);
			return lowerStr.find(m_Filter) != std::string::npos;
		}

	private:
		std::string m_Filter;
		bool m_CaseSensitive;
	};

	enum class ContentBrowserType : uint32_t
	{
		Asset,
		Filesystem
	};

	using OpenAssetCallbackFn = std::function<void(const AssetMetaData&)>;

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel(const std::string& panelName);
		~ContentBrowserPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnProjectChanged(Ref<Project> project) override;

		void ScheduleReload() { m_ReloadScheduled = true; }
		Ref<Project> GetProject() { return m_Project; }

		void RegisterOpenAssetCallback(AssetType assetType, const OpenAssetCallbackFn& func);

		Ref<Texture2D> GetDirectoryIcon() const { return m_FolderIcon; }
		Ref<Texture2D> GetFileIcon(const std::filesystem::path& filepath) const;
	private:
		void Reload();
		void CacheDirectories(Ref<DirectoryInfo> directory);

	private:
		void ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistory = true);
		void NextDirectory(Ref<DirectoryInfo> directory, bool addToHistory = true, bool clearSearch = true);

	private:
		CBItemList Search(const std::string& filterPaddern, Ref<DirectoryInfo> directory, bool searchSubdirectories = true);
		CBItemList GetItemsForAssetBrowser(Ref<DirectoryInfo> directory);
		CBItemList GetItemsForFileBrowser(Ref<DirectoryInfo> directory);

		Ref<DirectoryInfo> GetDirectory(const std::filesystem::path& filePath);
	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		void DrawItems();
		void DrawHeader();
		void DrawDirectoryHirachy(Ref<DirectoryInfo> directory);

		Ref<Texture2D> GetIcon(const AssetMetaData& metadata);
		Ref<Texture2D> GetThumbnail(const AssetMetaData& metadata);
		void GenerateThumbnails();

		bool IsSearchActive() { return m_SearchBuffer[0] != '\0'; }
		CBItemType GetItemTypeFromPath(const std::filesystem::path& path) const;

	private:
		Ref<ContentBrowserItem> CreateDirectory(Ref<DirectoryInfo> directory, const std::string& name, bool startRenaming);

		template<typename TAsset, typename... TArgs>
		void CreateAsset(Ref<DirectoryInfo> directory, const std::string& name, bool startRename, TArgs&&... args)
		{
			std::filesystem::path directoryPath = FileSystem::Relative(m_Project->GetAbsolute(directory->Filepath), m_Project->GetAssetsDirectory());
			Ref<EditorAssetManager> assetManager = m_Project->GetEditorAssetManager();
			Ref<TAsset> asset = assetManager->CreateAsset<TAsset>(directoryPath.string(), name, std::forward<TArgs>(args)...);
			Ref<ContentBrowserItem> newItem = Ref<ContentBrowserItem>::Create(this, CBItemType::Asset, assetManager->GetFilesystemPath(asset->Handle));
			m_CurrentItems.Add(newItem);
			directory->AddFile(name);

			if (startRename)
				newItem->StartRenaming();
		}

	private:
		Ref<Project> m_Project;
		ContentBrowserType m_BrowserType = ContentBrowserType::Asset;

		bool m_ReloadScheduled = true;
		bool m_PanelFocused = false;
		bool m_ChangesBlocked = false;

		Ref<DirectoryInfo> m_BaseDirectory;
		Ref<DirectoryInfo> m_CurrentDirectory;
		std::unordered_map<std::filesystem::path, Ref<DirectoryInfo>> m_DirectoryMap;

		bool m_ChangeDirectory = false;
		Ref<DirectoryInfo> m_NextDirectory = nullptr;
		bool m_AddNextToHistory = false;

		CBHistory m_History;
		std::vector<Ref<DirectoryInfo>> m_BreadcrumbTrailData;

		CBItemList m_CurrentItems;
		Ref<ContentBrowserItem> m_SelectedItem = nullptr;

		std::unordered_map<AssetType, OpenAssetCallbackFn> m_OpenAssetCallbacks;

		std::unordered_map<std::string, Ref<Texture2D>> m_IconExtensionMap;
		Ref<Texture2D> m_FileIcon;
		Ref<Texture2D> m_FolderIcon;

		enum { SearchBufferSize = 260 };
		char m_SearchBuffer[SearchBufferSize];
		bool m_SearchCaseSensitive = false;

		bool m_ShowInvalidFileNameError = false;
		float m_InvalidFileNameTimer = 0.0f;
		float m_InvalidFileNameTime = 4.0f; // 4s
		ImVec2 m_InvalidFileNameToolTipTopLeft;

		std::future<void> m_GenerateThumbnailsFuture;
		std::atomic<bool> m_StopGenerateThumbnails = false;

		static constexpr const char* s_BrowserTypeString[] = { "Asset", "Filesystem" };
	};

	constexpr std::string_view ToStringView(ContentBrowserType type)
	{
		switch (type)
		{
			case ContentBrowserType::Asset: return "Asset";
			case ContentBrowserType::Filesystem: return "Filesystem";
		}
	}

}
