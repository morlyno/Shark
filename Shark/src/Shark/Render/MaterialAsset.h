#pragma once

#include "Shark/Render/Material.h"

namespace Shark {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset(Ref<Material> material);
		~MaterialAsset();

		Ref<Material> GetMaterial();
		void SetMaterial(Ref<Material> material);

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetallic();
		void SetMetallic(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetAmbientOcclusion();
		void SetAmbientOcclusion(float value);

		Ref<Texture2D> GetAlbedoMap();
		void SetAlbedoMap(Ref<Texture2D> texture);
		bool UsingAlbedoMap();
		void SetUsingAlbedoMap(bool value);
		void ClearAlbedoMap();
	private:
		void SetDefault();
	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Material; }
		static Ref<MaterialAsset> Create(Ref<Material> material) { return Ref<MaterialAsset>::Create(material); }
	private:
		Ref<Material> m_Material;
	};

	class MaterialTable : public RefCount
	{
	public:
		using MaterialMap = std::map<uint32_t, Ref<MaterialAsset>>;

	public:
		MaterialTable() = default;
		~MaterialTable() = default;

		bool HasMaterial(uint32_t index) const { return m_Materials.contains(index); }
		void SetMaterial(uint32_t index, Ref<MaterialAsset> material);
		void ClearMaterial(uint32_t index);

		Ref<MaterialAsset> GetMaterial(uint32_t index) const;

		MaterialMap& GetMaterials() { return m_Materials; }
		const MaterialMap& GetMaterials() const { return m_Materials; }

		void Clear();

	private:
		MaterialMap m_Materials;

	};

}
