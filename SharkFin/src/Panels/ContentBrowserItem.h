#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Render/Image.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Shark {

	struct CBItemAction
	{
		enum Type : uint16_t
		{
			None = 0,
			Open = BIT(0),
			OpenInExplorer = BIT(1),
			Selected = BIT(2),
			Rename = BIT(3),
			Deleted = BIT(4),
			ReloadAsset = BIT(5),

			ReloadRequired = BIT(15)
		};
		using Flags = std::underlying_type_t<Type>;
	};

	enum class CBItemType : uint16_t
	{
		None = 0,
		Directory,
		Asset
	};

	class DirectoryInfo;

	class ContentBrowserItem : public RefCount
	{
		struct State
		{
			enum Type : uint16_t
			{
				None = 0,
				Renaming = BIT(0),
				StartRenaming = BIT(1),
				Selcted = BIT(2),
				Deleted = BIT(3)
			};
			using Flags = std::underlying_type_t<Type>;
		};
	public:
		ContentBrowserItem() = default;
		ContentBrowserItem(CBItemType type, AssetHandle handle, const std::string& name, Ref<Image2D> thumbnail);
		ContentBrowserItem(Ref<DirectoryInfo> directory);
		ContentBrowserItem(const AssetMetaData& metadata, Ref<Image2D> thumbnail);
		~ContentBrowserItem();

		CBItemType GetType() const { return m_Type; }
		AssetHandle GetHandle() const { return m_Handle; }
		const std::string& GetName() const { return m_Name; }

		CBItemAction::Flags Draw();

		void SetSelected(bool selected) { SetState(State::Selcted, selected); }
		bool IsSelected() const { return IsStateSet(State::Selcted); }

		void StartRenameing();
		void Rename(const std::string& name);
		void Delete();

		bool IsRenaming() const { return IsStateSet(State::Renaming); }

	private:
		void DrawPopup(CBItemAction::Flags& action);

		void SetState(State::Flags state, bool enabled);
		bool IsStateSet(State::Flags state) const;
		std::string GetTypeString();

	private:
		CBItemType m_Type = CBItemType::None;
		AssetHandle m_Handle = AssetHandle::Invalid;
		std::string m_Name;
		State::Flags m_State = State::None;

		Ref<Image2D> m_Thumbnail;

		bool m_IsHovered = false;

		enum { RenameBufferSize = 260 };
		inline static char RenameBuffer[RenameBufferSize];

		friend class ContentBrowserPanel;
	};

	class DirectoryInfo : public RefCount
	{
	public:
		DirectoryInfo(const std::filesystem::path& filePath, AssetHandle handle);
		DirectoryInfo(Ref<DirectoryInfo> parent, const std::filesystem::path& filePath, AssetHandle handle);
		~DirectoryInfo();
		void Reload();

		void AddDirectory(Ref<DirectoryInfo> directory);

		std::filesystem::path FilePath;
		AssetHandle Handle;

		Ref<DirectoryInfo> Parent;
		std::vector<Ref<DirectoryInfo>> SubDirectories;
		std::vector<AssetHandle> Assets;

		static Ref<DirectoryInfo> Create(Ref<DirectoryInfo> parent, const std::string& name, AssetHandle handle);
	};

}
