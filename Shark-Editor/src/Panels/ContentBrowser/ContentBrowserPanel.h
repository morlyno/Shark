#pragma once

#include "Shark.h"
#include "Shark/Core/Project.h"
#include "Shark/Event/Event.h"
#include "Panel.h"

#include "Panels/ContentBrowser/ContentBrowserItem.h"
#include "Panels/ContentBrowser/ThumbnailGenerator.h"
#include "Panels/ContentBrowser/ThumbnailCache.h"

#include <future>

#undef CreateDirectory
#undef RemoveDirectory
#undef GetCurrentDirectory

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
					return lhs->GetName() < rhs->GetName();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetName() < rhs->GetName();
			});
		}

		void Add(Ref<ContentBrowserItem> item)
		{
			const auto it = std::ranges::lower_bound(Items, item, [](auto lhs, auto rhs)
			{
				if (lhs->GetType() == rhs->GetType())
					return lhs->GetName() < rhs->GetName();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetName() < rhs->GetName();
			});
			Items.insert(it, item);
		}

		bool Contains(UUID id)
		{
			return std::ranges::find(Items, id, [](auto item) { return item->GetID(); }) != Items.end();
		}

		Ref<ContentBrowserItem> Get(UUID id)
		{
			return *std::ranges::find(Items, id, [](auto item) { return item->GetID(); });
		}

		Ref<ContentBrowserItem> TryGet(UUID id)
		{
			const auto it = std::ranges::find(Items, id, [](auto item) { return item->GetID(); });
			if (it != Items.end())
			{
				return *it;
			}
			return nullptr;
		}

		void Remove(UUID id)
		{
			const auto it = std::ranges::find(Items, id, [](auto item) { return item->GetID(); });
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
					return lhs->GetName() < rhs->GetName();

				if (lhs->GetType() == CBItemType::Directory)
					return true;

				if (rhs->GetType() == CBItemType::Directory)
					return false;

				return lhs->GetName() < rhs->GetName();
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

	using AssetActivatedCallbackFn = std::function<void(const AssetMetaData&)>;

	enum class ErrorType { None = 0, InvalidDirectory, InvalidInput, OperationFailed };
	enum class ErrorResponse { OK = BIT(0) };

	class ContentBrowserPanel : public Panel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel();

		virtual void OnImGuiRender(bool& shown) override;
		virtual void OnEvent(Event& event) override;
		virtual void OnProjectChanged(Ref<ProjectConfig> project) override;
		virtual void SetContext(Ref<Scene> context) override { m_SceneContext = context; }

		void ScheduleReload() { m_ReloadScheduled = true; }
		Ref<ProjectConfig> GetProject() const { return m_ProjectConfig; }
		Ref<ThumbnailCache> GetThumbnailCache() const { return m_ThumbnailCache; }
		Ref<DirectoryInfo> GetCurrentDirectory() const { return m_CurrentDirectory; }
		UUID GetSelectionID() const { return m_SelectionID; }

		void RegisterAssetActicatedCallback(AssetType assetType, const AssetActivatedCallbackFn& func);

		static const char* GetStaticID() { return "ContentBrowserPanel"; }
		virtual const char* GetPanelID() const override { return GetStaticID(); }
	private:
		void Reload();
		void CacheDirectories();
		void BuildDirectoryIDMap(Ref<DirectoryInfo> directory);
		void ParseDirectories(Ref<DirectoryInfo> directory);

		void ChangeDirectory(Ref<DirectoryInfo> directory, bool addToHistory = true);
		void GenerateThumbnails();

		CBItemList Search(const std::string& filterPaddern, Ref<DirectoryInfo> directory, bool searchSubdirectories = true);
		CBItemList GetItemsInDirectory(Ref<DirectoryInfo> directory);

	public: // ContentBrowserItem Interface
		Ref<DirectoryInfo> GetDirectory(UUID id) const;
		Ref<DirectoryInfo> FindDirectory(const std::filesystem::path& filePath);

		void NextDirectory(Ref<DirectoryInfo> directory, bool addToHistory = true, bool clearSearch = true);
		void SelectItem(UUID id, bool add = false);
		void ShowErrorDialogue(ErrorType type, const std::string& message, ErrorResponse response);
	private:
		bool OnKeyPressedEvent(KeyPressedEvent& event);

		void DrawItems();
		void DrawHeader();
		void DrawDirectoryHierarchy(Ref<DirectoryInfo> directory);

		bool IsSearchActive() { return m_SearchBuffer[0] != '\0'; }
		Ref<Texture2D> GetAssetIcon(const std::string& extension);
		void ApplySelectionRequests(ImGuiMultiSelectIO* selectionRequests, bool isBegin);
		void HandleSelectionRequests();

		void HandleEntityPayload(const ImGuiPayload* payload, Ref<DirectoryInfo> directory);

	private:
		Ref<ContentBrowserItem> CreateDirectory(const std::string& name, bool startRenaming);

		template<typename TAsset, typename... TArgs>
		void CreateAsset(const std::string& name, bool startRename, TArgs&&... args)
		{
			std::filesystem::path directoryPath = m_ProjectConfig->GetAbsolute(m_CurrentDirectory->Filepath / name);
			Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
			Ref<TAsset> asset = assetManager->CreateAsset<TAsset>(directoryPath, std::forward<TArgs>(args)...);
			const auto& metadata = assetManager->GetMetadata(asset);
			Ref<ContentBrowserItem> newItem = Ref<ContentBrowserAsset>::Create(this, metadata, GetAssetIcon(FileSystem::GetExtensionString(metadata.FilePath)));
			m_CurrentItems.Add(newItem);
			m_CurrentDirectory->AddFile(name);
			SelectItem(metadata.Handle);

			if (startRename)
				newItem->StartRenaming();
		}
		
		template<typename TAsset, typename... TArgs>
		void CreateAsset(Ref<DirectoryInfo> directory, const std::string& name, bool startRename, TArgs&&... args)
		{
			std::filesystem::path directoryPath = m_ProjectConfig->GetAbsolute(directory->Filepath / name);
			Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
			Ref<TAsset> asset = assetManager->CreateAsset<TAsset>(directoryPath, std::forward<TArgs>(args)...);
			const auto& metadata = assetManager->GetMetadata(asset);
			Ref<ContentBrowserItem> newItem = Ref<ContentBrowserAsset>::Create(this, metadata, GetAssetIcon(FileSystem::GetExtensionString(metadata.FilePath)));

			directory->AddFile(name);
			if (m_CurrentDirectory == directory)
			{
				m_CurrentItems.Add(newItem);
				m_CurrentDirectory->AddFile(name);
				SelectItem(metadata.Handle);

				if (startRename)
					newItem->StartRenaming();
			}
		}

	private:
		Ref<ProjectConfig> m_ProjectConfig;
		Ref<ThumbnailCache> m_ThumbnailCache;
		Ref<ThumbnailGenerator> m_ThumbnailGenerator;
		UUID m_SelectionID = UUID::Generate();
		Ref<Scene> m_SceneContext;

		CBItemList m_CurrentItems;

		Ref<DirectoryInfo> m_BaseDirectory;
		Ref<DirectoryInfo> m_CurrentDirectory;
		Ref<DirectoryInfo> m_NextDirectory = nullptr;

		CBHistory m_History;
		std::vector<Ref<DirectoryInfo>> m_BreadcrumbTrailData;

		ImGuiID m_DeleteDialoguePopupID = UI::GenerateUniqueID();
		ImGuiID m_ErrorDialogueID = UI::GenerateUniqueID();

		bool m_ReloadScheduled = true;
		bool m_PanelFocused = false;
		bool m_ChangesBlocked = false;

		bool m_ChangeDirectory = false;
		bool m_AddNextToHistory = false;

		bool m_ClearSelection = false;
		std::vector<UUID> m_ItemsToSelect;

		std::unordered_map<UUID, Ref<DirectoryInfo>> m_DirectoryMap;
		std::unordered_map<AssetType, AssetActivatedCallbackFn> m_AssetActivatedCallbacks;
		std::unordered_map<std::string, Ref<Texture2D>> m_IconExtensionMap;

		char m_SearchBuffer[260];
		bool m_SearchCaseSensitive = false;

		struct DeleteDialogueData
		{
			uint32_t SelectedCount = 0;
			std::vector<std::pair<UUID, bool>> Items;
		} m_DeleteDialogue;

		struct ErrorDialogue
		{
			std::string Title;
			std::string Message;
			ErrorType Type = ErrorType::None;
			ErrorResponse Response = ErrorResponse::OK;
		} m_ErrorDialogue;
	};

}
