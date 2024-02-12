#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Project.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Render/Texture.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	class ContentBrowserPanel;

	class DirectoryInfo : public RefCount
	{
	public:
		DirectoryInfo(Weak<DirectoryInfo> parent, const std::filesystem::path& directoryPath);
		~DirectoryInfo();

		void Reload(Ref<Project> project);

		void AddDirectory(Ref<DirectoryInfo> directory);
		void AddFile(const std::string& filename);

		void Rename(const std::string& newName);
		void RenameFile(const std::string& oldName, const std::string& newName);

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

		Open = BIT(0),
		OpenExternally = BIT(1),
		OpenInExplorer = BIT(2),
		Select = BIT(3),
		Delete = BIT(4),
		ReloadAsset = BIT(5),
		StartRenaming = BIT(6),
		FinishedRenaming = BIT(7),
		Remove = BIT(8),
		ImportFile = BIT(9)
	};

	class CBItemAction
	{
	public:
		bool FlagSet(CBItemActionFlag flag) const
		{
			return Flags & (uint16_t)flag;
		}

		const std::string& GetErrorMessage() const
		{
			return m_ErrorMsg;
		}

		const std::string& GetNewName() const
		{
			return m_NewName;
		}

	private:
		void SetFlag(CBItemActionFlag flag, bool set = true)
		{
			if (set)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}

		void Delete()
		{
			Flags = (uint16_t)CBItemActionFlag::Delete;
		}

		void ErrorPrompt(const std::string& errorMsg)
		{
			m_ErrorMsg = errorMsg;
		}

		void FinishRenaming(const std::string& newName)
		{
			m_NewName = newName;
			SetFlag(CBItemActionFlag::FinishedRenaming);
		}

	private:
		uint16_t Flags = 0;
		std::string m_ErrorMsg;
		std::string m_NewName;

		friend class ContentBrowserItem;
	};

	enum class CBItemType : uint16_t
	{
		None = 0,
		Directory,
		Asset,
		File
	};

	class ContentBrowserItem : public RefCount
	{
	private:
		enum class StateFlag
		{
			None = 0,
			StartRenaming = BIT(0),
			Renaming = BIT(1),
			Deleted = BIT(2),
			Selected = BIT(3)
		};

	public:
		ContentBrowserItem(Ref<ContentBrowserPanel> context, CBItemType type, const std::filesystem::path& filepath);
		~ContentBrowserItem();

		CBItemType GetType() const { return m_Type; }
		void SetType(CBItemType type);
		const std::filesystem::path& GetPath() const { return m_Path; }
		const std::string& GetName() const { return m_Name; }

		CBItemAction Draw();

	public:
		void StartRenaming();
		void Select() { SetFlag(StateFlag::Selected, true); }
		void Unselect() { SetFlag(StateFlag::Selected, false); }
		bool IsSelected() const { return FlagSet(StateFlag::Selected); }

		bool Rename(const std::string& newName);
		bool Delete();

	private:
		void SetFlag(StateFlag flag, bool set);
		bool FlagSet(StateFlag flag) const;

		std::string GetTypeString() const;
		void UpdateIcon();

	private:
		Weak<ContentBrowserPanel> m_Context;
		CBItemType m_Type;
		std::filesystem::path m_Path;
		std::string m_Name;

		Ref<Texture2D> m_Icon;
		Ref<Texture2D> m_Thumbnail;

		bool m_IsHovered = false;
		uint32_t m_StateFlags = 0;

		char m_RenameBuffer[260];

		friend class ContentBrowserPanel;
	};

}
