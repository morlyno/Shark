#pragma once

#include "Shark.h"
#include "Shark/Core/Project.h"
#include "Shark/Event/Event.h"
#include "Shark/Editor/Panel.h"

#include "Panels/ContentBrowser/ContentBrowserItem.h"

#undef CreateDirectory

namespace Shark {

	struct CBItemAssetHandleCompare
	{
		bool operator()(const Ref<ContentBrowserItem>& lhs, const Ref<ContentBrowserItem>& rhs) const
		{
			return lhs->GetHandle() == rhs->GetHandle();
		}
	};

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

		bool Contains(AssetHandle handle) const
		{
			return FindByHandle(handle) != Items.end();
		}

		void Add(Ref<ContentBrowserItem> item)
		{
			const auto i = std::lower_bound(Items.begin(), Items.end(), item, [](const auto& lhs, const auto& rhs)
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetName() < rhs->GetName();
				return lhs->GetType() == CBItemType::Directory;
			});
			Items.insert(i, item);

			SK_CORE_ASSERT(std::is_sorted(Items.begin(), Items.end(), [](const auto& lhs, const auto& rhs)
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetName() < rhs->GetName();
				return lhs->GetType() == CBItemType::Directory;
			}));
		}

		void Erase(AssetHandle handle)
		{
			Items.erase(FindByHandle(handle));
		}

		Ref<ContentBrowserItem> Get(AssetHandle handle) const
		{
			auto it = FindByHandle(handle);
			if (it == Items.end())
				return nullptr;
			return *it;
		}

		std::vector<Ref<ContentBrowserItem>>::const_iterator FindByHandle(AssetHandle handle) const
		{
			return std::find_if(Items.begin(), Items.end(), [handle](const Ref<ContentBrowserItem>& item) { return item->GetHandle() == handle; });
		}
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

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel(const char* panelName);
		~ContentBrowserPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;
		void OnFileEvents(const std::vector<FileChangedData>& fileEvents);

		void Reload() { m_ReloadScheduled = true; }

	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		void Internal_ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistroy);
		void Internal_MoveForward();
		void Internal_MoveBackward();
		void Internal_OnItemDeleted(Ref<ContentBrowserItem> item);
		void Internal_OnDirectoryDeleted(Ref<DirectoryInfo> directory);

		void ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistroy = true);
		void MoveForward();
		void MoveBackward();
		void OnItemSelcted(Ref<ContentBrowserItem> item);

		void SelectItem(Ref<ContentBrowserItem> item);

		void DrawItems();
		void DrawHeader();
		void DrawBreadcrumbTrail();
		void DrawDirectoryHirachy(Ref<DirectoryInfo> directory);
		CBItemList Search(const std::string& filter);
		void Search(const CBFilter& filter, Ref<DirectoryInfo> directory, std::vector<Ref<ContentBrowserItem>>& foundItmes);
		void CheckForProject();
		void CheckForReload();
		void CacheDirectoryHandles();
		void CacheDirectoryHandles(Ref<DirectoryInfo> directory);
		void ClearHistroy();

		Ref<Image2D> GetIcon(const AssetMetaData& metadata);
		Ref<Image2D> GetThumbnail(const AssetMetaData& metadata);
		void GenerateThumbnails();
		//Ref<DirectoryInfo> FindDirectory(const std::filesystem::path& filePath);
		Ref<DirectoryInfo> GetDirectory(AssetHandle handle);
		Ref<DirectoryInfo> GetDirectory(const std::filesystem::path& filePath);
		Ref<ProjectInstance> GetProject() const { return m_Project; }

		void SkipNextFileEvents() { m_SkipNextFileEvents = true; }
		bool IsSearchActive() { return m_SearchBuffer[0] != '\0'; }

	private:
		Ref<ContentBrowserItem> CreateDirectory(Ref<DirectoryInfo> directory, const std::string& name);

		template<typename TAsset>
		void CreateAsset(Ref<DirectoryInfo> directory, const std::string& name, bool startRename)
		{
			m_SkipNextFileEvents = true;
			std::filesystem::path directoryPath = std::filesystem::relative(m_Project->Directory / directory->FilePath, m_Project->AssetsDirectory);
			Ref<TAsset> asset = ResourceManager::CreateAsset<TAsset>(directoryPath.string(), name);
			const auto& metadata = ResourceManager::GetMetaData(asset);
			Ref<ContentBrowserItem> newItem = Ref<ContentBrowserItem>::Create(metadata, GetThumbnail(metadata));
			m_CurrentItems.Add(newItem);
			directory->Assets.emplace_back(metadata.Handle);

			if (startRename)
				newItem->StartRenameing();
		}


	private:
		static ContentBrowserPanel& Get() { return *s_Instance; }

	private:
		std::mutex m_Mutex;

		Ref<ProjectInstance> m_Project;
		Ref<ProjectInstance> m_NextProject;

		bool m_PanelFocused = false;

		bool m_ChangesBlocked = false;

		Ref<DirectoryInfo> m_BaseDirectory;
		Ref<DirectoryInfo> m_CurrentDirectory;

		CBItemList m_CurrentItems;
		Ref<ContentBrowserItem> m_SelectedItem = nullptr;

		std::vector<Ref<DirectoryInfo>> m_BreadcrumbTrailData;

		std::vector<Ref<DirectoryInfo>> m_History;
		uint32_t m_HistoryIndex = -1;

		std::unordered_map<AssetHandle, Ref<DirectoryInfo>> m_DirectoryHandleMap;
		std::unordered_map<std::string, Ref<Image2D>> m_IconExtensionMap;

		bool m_ReloadScheduled = true;

		enum { SearchBufferSize = 260 };
		char m_SearchBuffer[SearchBufferSize];
		bool m_SearchCaseSensitive = false;

		bool m_SkipNextFileEvents = false;

		bool m_ShowInvalidFileNameError = false;
		float m_InvalidFileNameTimer = 0.0f;
		float m_InvalidFileNameTime = 4.0f; // 4s
		ImVec2 m_InvalidFileNameToolTipTopLeft;

		inline static ContentBrowserPanel* s_Instance = nullptr;
		friend class DirectoryInfo;
		friend class ContentBrowserItem;

		std::vector<std::function<void()>> m_PostRenderQueue;
	};

}
