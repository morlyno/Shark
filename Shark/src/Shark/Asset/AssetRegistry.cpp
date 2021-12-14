#include "skpch.h"
#include "AssetRegistry.h"

#include "Shark/Utility/Utility.h"
#include "Shark/Utility/YAMLUtils.h"
#include "Shark/Core/Timer.h"

#include <yaml-cpp/yaml.h>

namespace Shark {

	AssetRegistry::AssetRegistry()
	{
	}

	AssetRegistry::~AssetRegistry()
	{
	}

	AssetMetaData& AssetRegistry::operator[](AssetHandle handle)
	{
		SK_CORE_ASSERT(handle.IsValid());
		return m_AssetMap[handle];
	}

	AssetMetaData& AssetRegistry::Get(AssetHandle handle)
	{
		SK_CORE_ASSERT(Utility::Contains(m_AssetMap, handle));
		return m_AssetMap.at(handle);
	}

	const AssetMetaData& AssetRegistry::Get(AssetHandle handle) const
	{
		SK_CORE_ASSERT(Utility::Contains(m_AssetMap, handle));
		return m_AssetMap.at(handle);
	}

	void AssetRegistry::Remove(AssetHandle handle)
	{
		m_AssetMap.erase(handle);
	}

	bool AssetRegistry::Contains(AssetHandle handle) const
	{
		return Utility::Contains(m_AssetMap, handle);
	}

	void AssetRegistry::Clear()
	{
		m_AssetMap.clear();
	}

	uint32_t AssetRegistry::Count() const
	{
		return (uint32_t)m_AssetMap.size();
	}

}

