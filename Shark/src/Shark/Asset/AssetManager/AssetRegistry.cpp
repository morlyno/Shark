#include "skpch.h"
#include "AssetRegistry.h"

namespace Shark {

	static AssetMetaData s_NullMetadata;

	bool AssetRegistry::Has(AssetHandle handle) const
	{
		std::scoped_lock lock(m_Mutex);
		return m_Registry.contains(handle);
	}

	AssetMetaData& AssetRegistry::Get(AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		return m_Registry.at(handle);
	}

	AssetMetaData& AssetRegistry::operator[](AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		return m_Registry[handle];
	}

	AssetMetaData& AssetRegistry::TryGet(AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		if (m_Registry.contains(handle))
			return m_Registry.at(handle);
		return s_NullMetadata;
	}

	const AssetMetaData& AssetRegistry::TryGet(AssetHandle handle) const
	{
		std::scoped_lock lock(m_Mutex);
		if (m_Registry.contains(handle))
			return m_Registry.at(handle);
		return s_NullMetadata;
	}

	void AssetRegistry::Remove(AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		m_Registry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		std::scoped_lock lock(m_Mutex);
		m_Registry.clear();
	}

	const AssetMetaData& AssetRegistry::Find(const std::filesystem::path& assetFilepath) const
	{
		std::scoped_lock lock(m_Mutex);
		for (const auto& [handle, metadata] : m_Registry)
			if (metadata.FilePath == assetFilepath)
				return metadata;
		return s_NullMetadata;
	}

}
