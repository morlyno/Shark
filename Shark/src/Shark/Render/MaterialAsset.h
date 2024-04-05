#pragma once

#include "Shark/Render/Material.h"

namespace Shark {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset();
		MaterialAsset(Ref<Material> material, bool setDefault = true);
		~MaterialAsset();

		void Invalidate();

		Ref<Material> GetMaterial();
		void SetMaterial(Ref<Material> material);

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		AssetHandle GetAlbedoMap();
		void SetAlbedoMap(AssetHandle handle);
		void ClearAlbedoMap();

		AssetHandle GetNormalMap();
		void SetNormalMap(AssetHandle handle);
		bool IsUsingNormalMap();
		void SetUsingNormalMap(bool value);
		void ClearNormalMap();

		AssetHandle GetMetalnessMap();
		void SetMetalnessMap(AssetHandle handle);
		void ClearMetalnessMap();
		
		AssetHandle GetRoughnessMap();
		void SetRoughnessMap(AssetHandle handle);
		void ClearRoughnessMap();

		void SetDefault();
	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Material; }
		static Ref<MaterialAsset> Create() { return Ref<MaterialAsset>::Create(); }
		static Ref<MaterialAsset> Create(Ref<Material> material, bool setDefault = true) { return Ref<MaterialAsset>::Create(material, setDefault); }

	private:
		Ref<Material> m_Material;

		AssetHandle m_AlbedoMap;
		AssetHandle m_NormalMap;
		AssetHandle m_MetalnessMap;
		AssetHandle m_RoughnessMap;
	};

	class MaterialTable : public RefCount
	{
	public:
		using MaterialMap = std::map<uint32_t, AssetHandle>;

	public:
		MaterialTable(uint32_t count = 1);
		~MaterialTable() = default;

		bool HasMaterial(uint32_t index) const { return m_Materials.contains(index); }
		void SetMaterial(uint32_t index, AssetHandle material);
		void ClearMaterial(uint32_t index);

		AssetHandle GetMaterial(uint32_t index) const;

		MaterialMap& GetMaterials() { return m_Materials; }
		const MaterialMap& GetMaterials() const { return m_Materials; }

		uint32_t& GetMaterialCount() { return m_MaterialCount; }
		uint32_t GetMaterialCount() const { return m_MaterialCount; }

		void Clear();

	private:
		uint32_t m_MaterialCount = 0;
		MaterialMap m_Materials;

	};

}
