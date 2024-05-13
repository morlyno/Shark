#include "skpch.h"
#include "AssetRegistry.h"

namespace Shark {

	static AssetMetaData s_NullMetadata;

	bool AssetRegistry::Has(AssetHandle handle) const
	{
		return m_Registry.contains(handle);
	}

	AssetMetaData& AssetRegistry::Get(AssetHandle handle)
	{
		return m_Registry.at(handle);
	}

	const AssetMetaData& AssetRegistry::Get(AssetHandle handle) const
	{
		return m_Registry.at(handle);
	}

	AssetMetaData& AssetRegistry::operator[](AssetHandle handle)
	{
		return m_Registry[handle];
	}

	AssetMetaData& AssetRegistry::TryGet(AssetHandle handle)
	{
		if (m_Registry.contains(handle))
			return m_Registry.at(handle);
		return s_NullMetadata;
	}

	const AssetMetaData& AssetRegistry::TryGet(AssetHandle handle) const
	{
		if (m_Registry.contains(handle))
			return m_Registry.at(handle);
		return s_NullMetadata;
	}

	void AssetRegistry::Remove(AssetHandle handle)
	{
		m_Registry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		m_Registry.clear();
	}

	const AssetMetaData& AssetRegistry::Find(const std::filesystem::path& assetFilepath) const
	{
		for (const auto& [handle, metadata] : m_Registry)
			if (metadata.FilePath == assetFilepath)
				return metadata;
		return s_NullMetadata;
	}

}
