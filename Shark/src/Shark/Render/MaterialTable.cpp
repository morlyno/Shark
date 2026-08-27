#include "skpch.h"
#include "MaterialTable.h"

namespace Shark {

	MaterialTable::MaterialTable(uint32_t slots)
		: m_MaterialSlots(slots)
	{
	}

	void MaterialTable::SetMaterial(uint32_t index, AssetHandle material)
	{
		m_Materials[index] = material;
		if (index >= m_MaterialSlots)
			m_MaterialSlots = index + 1;
	}

	void MaterialTable::ClearMaterial(uint32_t index)
	{
		if (!HasMaterial(index))
			return;
		m_Materials.erase(index);
		if (index >= m_MaterialSlots)
			m_MaterialSlots = index + 1;
	}

	AssetHandle MaterialTable::GetMaterial(uint32_t index) const
	{
		SK_CORE_VERIFY(HasMaterial(index));
		return m_Materials.at(index);
	}

	void MaterialTable::Clear()
	{
		m_Materials.clear();
	}

}
