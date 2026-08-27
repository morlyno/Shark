#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/AssetTypes.h"

namespace Shark {

	class MaterialTable : public RefCount
	{
	public:
		using MaterialMap = std::map<uint32_t, AssetHandle>;

	public:
		MaterialTable(uint32_t slots = 1);
		~MaterialTable() = default;

		bool HasMaterial(uint32_t index) const { return m_Materials.contains(index); }
		void SetMaterial(uint32_t index, AssetHandle material);
		void ClearMaterial(uint32_t index);

		AssetHandle GetMaterial(uint32_t index) const;

		MaterialMap& GetMaterials() { return m_Materials; }
		const MaterialMap& GetMaterials() const { return m_Materials; }

		uint32_t GetSlotCount() const { return m_MaterialSlots; }
		void SetSlotCount(uint32_t count) { m_MaterialSlots = count; }

		void Clear();

	private:
		uint32_t m_MaterialSlots = 0;
		MaterialMap m_Materials;

	};

}
