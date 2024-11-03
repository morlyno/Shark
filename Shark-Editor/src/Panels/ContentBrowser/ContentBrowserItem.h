#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Render/Texture.h"

#include <imgui.h>

#undef CreateDirectory
#undef RemoveDirectory

namespace Shark {

	class ContentBrowserPanel;

	class DirectoryInfo : public RefCount
	{
	public:
		DirectoryInfo(Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath);
		DirectoryInfo(UUID id, Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath);
		~DirectoryInfo();

		void AddDirectory(Ref<DirectoryInfo> directory);
		void AddFile(const std::string& filename);

		void RemoveFile(const std::string& filename);
		void RemoveDirectory(Ref<DirectoryInfo> directory);
		void RemoveDirectory(const std::filesystem::path& dirPath);

		void Rename(const std::string& newName);
		void RenameFile(const std::string& oldName, const std::string& newName);

		UUID ID;
		std::filesystem::path Filepath;
		std::string Name;

		Weak<DirectoryInfo> Parent;
		std::vector<Ref<DirectoryInfo>> SubDirectories;

		// Only Filenames
		std::vector<std::string> Filenames;
	};

	enum class CBItemActionFlag : uint16_t
	{
		None = 0,
		Refresh = BIT(0),
		Reload = BIT(1),
		Renamed = BIT(2),
		StartRenaming = BIT(3),
		OpenDeleteDialogue = BIT(4),
		OpenExternal = BIT(5),
		ShowInExplorer = BIT(6),
		Activated = BIT(7),
		DropAccepted = BIT(8)
	};

	struct CBItemAction
	{
		bool IsSet(CBItemActionFlag flag) const
		{
			return (Flags & flag) == flag;
		}

		void Set(CBItemActionFlag flag, bool set = true)
		{
			if (set)
				Flags |= flag;
			else
				Flags &= ~flag;
		}

		CBItemActionFlag Flags = CBItemActionFlag::None;
	};

	enum class CBItemType
	{
		Directory, Asset
	};

	class ContentBrowserItem : public RefCount
	{
	public:
		ContentBrowserItem(Ref<ContentBrowserPanel> context, CBItemType type, UUID id, const std::string& name, Ref<Texture2D> icon);
		~ContentBrowserItem();

		CBItemAction Draw();

		CBItemType GetType() const { return m_Type; }
		UUID GetID() const { return m_ID; }
		const std::string& GetName() const { return m_FileName; }
		const std::string& GetDisplayName() const { return m_DisplayName; }
		std::string_view GetSpecificTypeName() const;

		void StartRenaming();
		void StopRenaming();
		bool IsRenaming() const { return m_IsRenameing; }

		virtual bool Move(Ref<DirectoryInfo> destinationDirectory) = 0;
		virtual bool Rename(const std::string& newName) = 0;
		virtual bool Delete() = 0;

		void SetDisplayNameFromFileName();

	public:
		void OnRenamed(const std::string& newFilename);
		virtual void DrawCustomContextItems() {}
		virtual void UpdateDragDrop(CBItemAction& action) {}

	private:
		void DrawPopupMenu(CBItemAction& action);

	protected:
		Weak<ContentBrowserPanel> m_Context;
		CBItemType m_Type;
		UUID m_ID;
		std::string m_FileName;
		std::string m_DisplayName;
		Ref<Texture2D> m_Icon;

		bool m_IsRenameing = false;
	};

	class ContentBrowserAsset : public ContentBrowserItem
	{
	public:
		ContentBrowserAsset(Ref<ContentBrowserPanel> context, const AssetMetaData& metadata, Ref<Texture2D> icon);
		~ContentBrowserAsset();

		virtual bool Move(Ref<DirectoryInfo> destinationDirectory) override;
		virtual bool Rename(const std::string& newName) override;
		virtual bool Delete() override;

		virtual void DrawCustomContextItems() override;
		virtual void UpdateDragDrop(CBItemAction& action) override;

		const AssetMetaData& GetMetadata() const { return m_Metadata; }
	private:
		const AssetMetaData& m_Metadata;
	};

	class ContentBrowserDirectory : public ContentBrowserItem
	{
	public:
		ContentBrowserDirectory(Ref<ContentBrowserPanel> context, Ref<DirectoryInfo> directoryInfo);
		~ContentBrowserDirectory();

		virtual bool Move(Ref<DirectoryInfo> destinationDirectory) override;
		virtual bool Rename(const std::string& newName) override;
		virtual bool Delete() override;

		virtual void UpdateDragDrop(CBItemAction& action) override;

		Ref<DirectoryInfo> GetDirectoryInfo() const { return m_DirectoryInfo; }
	private:
		void UpdateSubdirectories(Ref<DirectoryInfo> directory);
		void DeleteSubdirectories(Ref<DirectoryInfo> directory);
	private:
		Ref<DirectoryInfo> m_DirectoryInfo;
	};

}
