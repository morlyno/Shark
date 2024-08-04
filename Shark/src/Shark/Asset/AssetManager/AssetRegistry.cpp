#include "skpch.h"
#include "AssetRegistry.h"

namespace Shark {

	bool AssetRegistry::Contains(AssetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return m_Registry.contains(handle);
	}

	void AssetRegistry::Add(const AssetMetaData& metadata)
	{
		std::scoped_lock lock(m_Mutex);
		SK_CORE_VERIFY(!m_Registry.contains(metadata.Handle));
		m_Registry.emplace(metadata.Handle, metadata);
	}

	void AssetRegistry::Update(const AssetMetaData& metadata)
	{
		std::shared_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(metadata.Handle));
		m_Registry.at(metadata.Handle) = metadata;
	}

	void AssetRegistry::Remove(AssetHandle handle)
	{
		std::scoped_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(handle));
		m_Registry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		std::scoped_lock lock(m_Mutex);
		m_Registry.clear();
	}

	AssetMetaData& AssetRegistry::Get(AssetHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	const AssetMetaData& AssetRegistry::Read(AssetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	AssetMetaData AssetRegistry::ReadCopy(AssetHandle handle)
	{
		std::shared_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	AssetMetaData AssetRegistry::ReadCopy(AssetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		SK_CORE_VERIFY(m_Registry.contains(handle));
		return m_Registry.at(handle);
	}

	AssetType AssetRegistry::ReadType(AssetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		return Read(handle).Type;
	}

	std::optional<AssetMetaData> AssetRegistry::TryReadCopy(AssetHandle handle) const
	{
		std::shared_lock lock(m_Mutex);
		if (m_Registry.contains(handle))
			return m_Registry.at(handle);
		return std::nullopt;
	}

	AssetHandle AssetRegistry::TryFind(const std::filesystem::path& assetFilepath) const
	{
		std::shared_lock lock(m_Mutex);
		for (const auto& [handle, metadata] : m_Registry)
			if (metadata.FilePath == assetFilepath)
				return metadata.Handle;
		return AssetHandle::Invalid;
	}

	void AssetRegistry::Lock() const
	{
		m_Mutex.lock();
	}

	void AssetRegistry::Unlock() const
	{
		m_Mutex.lock();
	}

	void AssetRegistry::LockRead() const
	{
		m_Mutex.lock_shared();
	}

	void AssetRegistry::UnlockRead() const
	{
		m_Mutex.unlock_shared();
	}

	std::unique_lock<std::shared_mutex> AssetRegistry::LockScoped() const
	{
		return std::unique_lock(m_Mutex);
	}

	std::shared_lock<std::shared_mutex> AssetRegistry::LockReadScoped() const
	{
		return std::shared_lock(m_Mutex);
	}

}
