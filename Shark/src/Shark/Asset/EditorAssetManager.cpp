#include "skpch.h"
#include "EditorAssetManager.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetSerializer.h"
#include "Shark/Asset/AssetUtils.h"
#include "Shark/File/FileSystem.h"

#include <yaml-cpp/yaml.h>
#include "Shark/Utils/YAMLUtils.h"

namespace Shark {

	namespace utils {
		
		static std::filesystem::path GetFilesystemKey(const std::filesystem::path& path)
		{
			return std::filesystem::absolute(path).lexically_normal();
		}

	}

	EditorAssetManager::EditorAssetManager(Ref<Project> project)
		: m_Project(project)
	{
		AssetSerializer::RegisterSerializers();
		ReadImportedAssetsFromDisc();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		WriteImportedAssetsToDisc();
		AssetSerializer::ReleaseSerializers();
	}

	void EditorAssetManager::SerializeImportedAssets()
	{
		WriteImportedAssetsToDisc();
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
	{
		AssetMetaData& metadata = GetMetadataInternal(handle);
		if (!metadata.IsValid())
			return nullptr;

		if (metadata.IsMemoryAsset)
			return m_LoadedAssets.at(handle);

		if (!metadata.IsDataLoaded)
		{
			Ref<Asset> asset = nullptr;
			metadata.IsDataLoaded = AssetSerializer::TryLoadAsset(asset, metadata);
			if (!metadata.IsDataLoaded)
				return nullptr;

			m_LoadedAssets[handle] = asset;
			return asset;
		}

		return m_LoadedAssets.at(handle);
	}

	AssetHandle EditorAssetManager::AddMemoryAsset(Ref<Asset> asset)
	{
		if (IsValidAssetHandle(asset->Handle))
		{
			SK_CORE_ERROR_TAG("AssetManager", "Tried to add a Memory Asset but Asset has a valid AssetHandle");
			return AssetHandle::Invalid;
		}

		AssetHandle newHandle = asset->Handle;
		if (newHandle == AssetHandle::Invalid)
			newHandle = AssetHandle::Generate();

		AssetMetaData metadata;
		metadata.Type = asset->GetAssetType();
		metadata.Handle = newHandle;
		metadata.IsMemoryAsset = true;
		metadata.IsDataLoaded = true;
		m_ImportedAssets[metadata.Handle] = metadata;

		asset->Handle = metadata.Handle;
		m_LoadedAssets[metadata.Handle] = asset;

		SK_CORE_INFO_TAG("AssetManager", "Memory Asset Added (Type: {}, Handle: 0x{:x})", ToString(metadata.Type), metadata.Handle);
		return metadata.Handle;
	}

	bool EditorAssetManager::IsValidAssetHandle(AssetHandle handle) const
	{
		return handle != AssetHandle::Invalid && m_ImportedAssets.contains(handle);
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle handle) const
	{
		return m_LoadedAssets.contains(handle);
	}

	bool EditorAssetManager::IsFileImported(const std::filesystem::path& filepath) const
	{
		const auto& metadata = GetMetadata(filepath);
		return metadata.IsValid();
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(AssetHandle handle) const
	{
		return GetMetadataInternal(handle);
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(Ref<Asset> asset) const
	{
		return GetMetadataInternal(asset->Handle);
	}

	const AssetMetaData& EditorAssetManager::GetMetadata(const std::filesystem::path& filepath) const
	{
		AssetHandle handle = GetAssetHandleFromFilePath(filepath);
		return GetMetadataInternal(handle);
	}

	std::filesystem::path EditorAssetManager::GetFilesystemPath(AssetHandle handle) const
	{
		const auto& metadata = GetMetadataInternal(handle);
		return GetFilesystemPath(metadata);
	}

	std::filesystem::path EditorAssetManager::GetFilesystemPath(const AssetMetaData& metadata) const
	{
		if (metadata.FilePath.empty())
			return {};

		if (metadata.IsEditorAsset)
			return FileSystem::GetFilesystemPath(metadata.FilePath);

		SK_CORE_VERIFY(!m_Project.Expired());
		return (m_Project.GetRef()->GetAssetsDirectory() / metadata.FilePath).lexically_normal();
	}

	std::filesystem::path EditorAssetManager::MakeRelativePath(const std::filesystem::path& filepath) const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return std::filesystem::relative(m_Project.GetRef()->GetActiveAssetsDirectory() / filepath, m_Project.GetRef()->GetAssetsDirectory());
	}

	std::string EditorAssetManager::MakeRelativePathString(const std::filesystem::path& filepath) const
	{
		return MakeRelativePath(filepath).string();
	}

	std::filesystem::path EditorAssetManager::GetProjectPath(const AssetMetaData& metadata) const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return m_Project.GetRef()->GetRelative(GetFilesystemPath(metadata));
	}

	bool EditorAssetManager::HasExistingFilePath(const AssetMetaData& metadata)
	{
		auto filesystemPath = GetFilesystemPath(metadata);
		return FileSystem::Exists(filesystemPath);
	}

	bool EditorAssetManager::HasExistingFilePath(AssetHandle handle)
	{
		auto filesystemPath = GetFilesystemPath(handle);
		return FileSystem::Exists(filesystemPath);
	}

	bool EditorAssetManager::LoadAsset(AssetHandle handle)
	{
		auto& metadata = GetMetadataInternal(handle);
		if (!metadata.IsDataLoaded && !metadata.IsMemoryAsset)
		{
			Ref<Asset> asset = nullptr;
			metadata.IsDataLoaded = AssetSerializer::TryLoadAsset(asset, metadata);
			if (metadata.IsDataLoaded)
				m_LoadedAssets[handle] = asset;
		}
		return metadata.IsDataLoaded;
	}

	bool EditorAssetManager::SaveAsset(AssetHandle handle)
	{
		const auto& metadata = GetMetadataInternal(handle);
		if (metadata.IsMemoryAsset || !metadata.IsDataLoaded)
			return false;

		return AssetSerializer::Serialize(m_LoadedAssets.at(handle), metadata);
	}

	bool EditorAssetManager::ReloadAsset(AssetHandle handle)
	{
		AssetMetaData& metadata = GetMetadataInternal(handle);
		if (metadata.IsMemoryAsset)
			return false;

		if (metadata.IsDataLoaded)
		{
			Ref<Asset> asset = m_LoadedAssets.at(handle);
			SK_CORE_ASSERT(asset);
			asset->SetFlag(AssetFlag::Invalid, true);
			UnloadAsset(handle);
		}

		return LoadAsset(handle);
	}

	void EditorAssetManager::UnloadAsset(AssetHandle handle)
	{
		auto& metadata = GetMetadataInternal(handle);
		if (!metadata.IsDataLoaded)
			return;

		// TODO(moro): remove AssetFlag
		if (auto asset = GetAsset(handle))
			asset->SetFlag(AssetFlag::Invalid, true);

		metadata.IsDataLoaded = false;
		m_LoadedAssets.erase(handle);

		if (metadata.IsMemoryAsset)
			m_ImportedAssets.erase(handle);
	}

	void EditorAssetManager::RemoveAsset(AssetHandle handle)
	{
		AssetMetaData metadata = GetMetadataInternal(handle);

		m_LoadedAssets.erase(handle);
		m_ImportedAssets.erase(handle);

		if (!metadata.IsMemoryAsset)
			WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("ResourceManager", "Asset Removed => Handle: 0x{:x}, Type: {}, FilePath: {}", handle, ToString(metadata.Type), metadata.FilePath);
	}

	bool EditorAssetManager::ImportMemoryAsset(AssetHandle handle, const std::string& directory, const std::string& filename)
	{
		auto& metadata = GetMetadataInternal(handle);
		if (!metadata.IsValid())
			return false;

		metadata.FilePath = MakeRelativePath(directory + "/" + filename);
		metadata.IsMemoryAsset = false;

		if (std::filesystem::exists(metadata.FilePath))
		{
			uint32_t count = 1;
			bool foundValidFilePath = false;
			while (!foundValidFilePath)
			{
				metadata.FilePath = fmt::format("{}/{} ({:2})", directory, filename, count++);
				foundValidFilePath = !std::filesystem::exists(metadata.FilePath);
			}
		}

		Ref<Asset> asset = m_LoadedAssets.at(handle);
		if (!AssetSerializer::Serialize(asset, metadata))
		{
			metadata.FilePath.clear();
			metadata.IsMemoryAsset = true;
			return false;
		}

		WriteImportedAssetsToDisc();
		return true;
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		AssetType type = AssetUtils::GetAssetTypeFromPath(filepath);
		if (type == AssetType::None)
			return 0;

		auto fsPath = FileSystem::GetFilesystemPath(filepath);
		if (!FileSystem::Exists(fsPath))
			return AssetHandle::Invalid;

		AssetHandle handle = GetAssetHandleFromFilePath(fsPath);
		if (handle != AssetHandle::Invalid)
			return handle;

		AssetMetaData metadata;
		metadata.Handle = AssetHandle::Generate();
		metadata.FilePath = MakeRelativePath(fsPath);
		metadata.Type = type;
		metadata.IsDataLoaded = false;
		m_ImportedAssets[metadata.Handle] = metadata;
		WriteImportedAssetsToDisc();

		SK_CORE_INFO_TAG("ResourceManager", "Imported Asset => Handle: 0x{:x}, Type: {}, FilePath: {}", metadata.Handle, ToString(metadata.Type), metadata.FilePath);
		return metadata.Handle;
	}

	const AssetsMap& EditorAssetManager::GetLoadedAssets() const
	{
		return m_LoadedAssets;
	}

	const AssetMetadataMap& EditorAssetManager::GetAssetMetadataMap() const
	{
		return m_ImportedAssets;
	}

	void EditorAssetManager::OnAssetCreated(const std::filesystem::path& filepath)
	{
		if (!AssetExtensionMap.contains(filepath.extension().string()))
			return;

		ImportAsset(filepath);
	}

	void EditorAssetManager::OnAssetDeleted(const std::filesystem::path& filepath)
	{
		AssetHandle handle = GetAssetHandleFromFilePath(filepath);
		if (handle != AssetHandle::Invalid)
			RemoveAsset(handle);
	}

	void EditorAssetManager::OnAssetRenamed(const std::filesystem::path& oldFilepath, const std::string& newName)
	{
		AssetHandle handle = GetAssetHandleFromFilePath(oldFilepath);
		AssetMetaData& metadata = GetMetadataInternal(handle);
		if (metadata.IsValid())
			return;

		std::filesystem::path newNamePath = newName;
		newNamePath.replace_extension(metadata.FilePath.extension());
		metadata.FilePath.replace_filename(newNamePath);
		WriteImportedAssetsToDisc();
	}

	AssetHandle EditorAssetManager::GetEditorAsset(const std::filesystem::path& filepath)
	{
		// TODO(moro): handle non existing file
		SK_CORE_VERIFY(FileSystem::Exists(filepath));

		const auto key = utils::GetFilesystemKey(filepath);
		if (!m_EditorAssets.contains(key))
			m_EditorAssets[key] = AssetHandle::Generate();

		AssetHandle handle = m_EditorAssets.at(key);
		SK_CORE_VERIFY(handle != AssetHandle::Invalid);
		if (!m_ImportedAssets.contains(handle))
		{
			AssetMetaData metadata;
			metadata.FilePath = filepath;
			metadata.Handle = handle;
			metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
			metadata.IsEditorAsset = true;
			m_ImportedAssets[handle] = metadata;
		}

		return handle;
	}

	AssetHandle EditorAssetManager::AddEditorAsset(AssetHandle handle, const std::filesystem::path& filepath)
	{
		const auto key = utils::GetFilesystemKey(filepath);
		if (m_EditorAssets.contains(key))
			return m_EditorAssets.at(key);

		m_EditorAssets[key] = handle;
		if (m_ImportedAssets.contains(handle))
			return handle;

		AssetMetaData metadata;
		metadata.FilePath = filepath;
		metadata.Handle = handle;
		metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
		metadata.IsEditorAsset = true;
		m_ImportedAssets[handle] = metadata;
		return handle;
	}

	AssetHandle EditorAssetManager::AddEditorAsset(Ref<Asset> asset, const std::filesystem::path& filepath)
	{
		const auto key = utils::GetFilesystemKey(filepath);
		if (m_EditorAssets.contains(key))
			return m_EditorAssets.at(key);

		if (asset->Handle == AssetHandle::Invalid)
			asset->Handle = AssetHandle::Generate();

		m_EditorAssets[key] = asset->Handle;
		if (m_ImportedAssets.contains(asset->Handle))
			return asset->Handle;

		AssetMetaData metadata;
		metadata.FilePath = filepath;
		metadata.Handle = asset->Handle;
		metadata.Type = AssetUtils::GetAssetTypeFromPath(filepath);
		metadata.IsEditorAsset = true;
		metadata.IsDataLoaded = true;
		m_ImportedAssets[asset->Handle] = metadata;
		m_LoadedAssets[asset->Handle] = asset;

		return asset->Handle;
	}

	bool EditorAssetManager::HasEditorAsset(const std::filesystem::path& filepath) const
	{
		const auto key = utils::GetFilesystemKey(filepath);
		return m_EditorAssets.contains(key);
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle handle) const
	{
		const AssetMetaData& metadata = GetMetadataInternal(handle);
		if (metadata.IsValid())
			return metadata.Type;
		return AssetType::None;
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle handle) const
	{
		const auto& metadata = GetMetadataInternal(handle);
		return metadata.IsMemoryAsset;
	}

	static AssetMetaData s_NullMetadata;
	AssetMetaData& EditorAssetManager::GetMetadataInternal(AssetHandle handle)
	{
		if (m_ImportedAssets.contains(handle))
			return m_ImportedAssets.at(handle);
		return s_NullMetadata;
	}

	const AssetMetaData& EditorAssetManager::GetMetadataInternal(AssetHandle handle) const
	{
		if (m_ImportedAssets.contains(handle))
			return m_ImportedAssets.at(handle);
		return s_NullMetadata;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath) const
	{
		auto relativePath = MakeRelativePath(filepath);
		for (const auto& [handle, metadata] : m_ImportedAssets)
			if (metadata.FilePath == relativePath)
				return metadata.Handle;
		return AssetHandle::Invalid;
	}

	struct ImportedAssetsEntry
	{
		AssetHandle Handle;
		AssetType Type;
		std::filesystem::path FilePath;

		ImportedAssetsEntry() = default;
		ImportedAssetsEntry(const AssetMetaData& metadata)
			: Handle(metadata.Handle), Type(metadata.Type), FilePath(metadata.FilePath) {}

		bool operator<(const ImportedAssetsEntry& rhs) const
		{
			if (Type == rhs.Type)
			{
				if (FilePath == rhs.FilePath)
					return Handle < rhs.Handle;
				return FilePath < rhs.FilePath;
			}
			return Type < rhs.Type;
		}
	};

	void EditorAssetManager::WriteImportedAssetsToDisc()
	{
		if (m_Project.Expired())
			return;

		auto project = m_Project.GetRef();
		const auto filepath = project->GetDirectory() / "ImportedAssets.yaml";

		std::set<ImportedAssetsEntry> sortedAssets;
		std::set<ImportedAssetsEntry> editorAssets;

		for (const auto& [handle, metadata] : m_ImportedAssets)
		{
			if (!metadata.IsValid() || metadata.IsMemoryAsset || !HasExistingFilePath(metadata))
				continue;

			if (metadata.IsEditorAsset)
			{
				editorAssets.emplace(metadata);
				continue;
			}

			sortedAssets.emplace(metadata);
		}

		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "ImportedAssets" << YAML::Value << YAML::Node(sortedAssets);
		out << YAML::Key << "EditorAssets" << YAML::Value << YAML::Node(editorAssets);
		out << YAML::EndMap;

		if (!FileSystem::WriteString(filepath, out.c_str()))
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to write imported assets to disc!");
			return;
		}
	}

	void EditorAssetManager::ReadImportedAssetsFromDisc()
	{
		SK_CORE_VERIFY(!m_Project.Expired());

		auto project = m_Project.GetRef();
		const auto filepath = m_Project.GetRef()->GetDirectory() / "ImportedAssets.yaml";

		if (!FileSystem::Exists(filepath))
		{
			SK_CORE_ERROR_TAG("Serialization", "ImportedAssets.yaml not found");
			return;
		}

		std::string filedata = FileSystem::ReadString(filepath);
		YAML::Node fileNode = YAML::Load(filedata);

		m_ImportedAssets.clear();

		auto assetsNode = fileNode["ImportedAssets"];
		if (!assetsNode)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to find ImportedAssets root node in ImportedAssets.yaml");
			return;
		}

		for (auto entryNode : assetsNode)
		{
			auto entry = entryNode.as<ImportedAssetsEntry>();

			AssetMetaData metadata;
			metadata.Handle = entry.Handle;
			metadata.Type = entry.Type;
			metadata.FilePath = entry.FilePath;

			if (!HasExistingFilePath(metadata))
				continue;

			SK_CORE_VERIFY(m_ImportedAssets.contains(metadata.Handle) == false);
			m_ImportedAssets[metadata.Handle] = metadata;
		}

		auto editorAssetsNode = fileNode["EditorAssets"];
		if (!editorAssetsNode)
		{
			SK_CORE_ERROR_TAG("Serialization", "Failed to find EditorAssets root node im ImportedAssets.yaml");
			return;
		}

		for (auto entryNode : editorAssetsNode)
		{
			auto entry = entryNode.as<ImportedAssetsEntry>();
			AssetMetaData metadata;
			metadata.Handle = entry.Handle;
			metadata.Type = entry.Type;
			metadata.FilePath = entry.FilePath;
			metadata.IsEditorAsset = true;

			if (!FileSystem::Exists(metadata.FilePath))
				continue;

			SK_CORE_VERIFY(m_ImportedAssets.contains(metadata.Handle) == false);
			m_ImportedAssets[metadata.Handle] = metadata;
			m_EditorAssets[utils::GetFilesystemKey(metadata.FilePath)] = metadata.Handle;
		}
	}

	const std::filesystem::path EditorAssetManager::GetAssetsDirectoryFromProject() const
	{
		SK_CORE_VERIFY(!m_Project.Expired());
		return m_Project.GetRef()->GetAssetsDirectory();
	}

}

namespace YAML {

	template<>
	struct convert<Shark::ImportedAssetsEntry>
	{
		static Node encode(const Shark::ImportedAssetsEntry& entry)
		{
			Node node(NodeType::Map);
			node.force_insert("Handle", entry.Handle);
			node.force_insert("Type", Shark::ToString(entry.Type));
			node.force_insert("FilePath", entry.FilePath);
			return node;
		}

		static bool decode(const Node& node, Shark::ImportedAssetsEntry& outEntry)
		{
			if (!node.IsMap() || node.size() != 3)
				return false;

			outEntry.Handle = node["Handle"].as<Shark::AssetHandle>();
			outEntry.Type = Shark::StringToAssetType(node["Type"].as<std::string>());
			outEntry.FilePath = node["FilePath"].as<std::filesystem::path>();
			return true;
		}
	};

}