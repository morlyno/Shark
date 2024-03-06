#pragma once

#include "Shark/Render/Material.h"

namespace Shark {

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset() = default;
		MaterialAsset(Ref<Material> material, bool setDefault = true);
		~MaterialAsset();

		Ref<Material> GetMaterial();
		void SetMaterial(Ref<Material> material);

		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		float& GetMetalness();
		void SetMetalness(float value);

		float& GetRoughness();
		void SetRoughness(float value);

		float& GetAmbientOcclusion();
		void SetAmbientOcclusion(float value);

		Ref<Texture2D> GetAlbedoMap();
		void SetAlbedoMap(Ref<Texture2D> texture);
		void ClearAlbedoMap();

		Ref<Texture2D> GetNormalMap();
		void SetNormalMap(Ref<Texture2D> texture);
		bool IsUsingNormalMap();
		void SetUsingNormalMap(bool value);
		void ClearNormalMap();

		Ref<Texture2D> GetMetalnessMap();
		void SetMetalnessMap(Ref<Texture2D> texture);
		void ClearMetalnessMap();
		
		Ref<Texture2D> GetRoughnessMap();
		void SetRoughnessMap(Ref<Texture2D> texture);
		void ClearRoughnessMap();

		void SetDefault();
	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Material; }
		static Ref<MaterialAsset> Create() { return Ref<MaterialAsset>::Create(); }
		static Ref<MaterialAsset> Create(Ref<Material> material, bool setDefault = true) { return Ref<MaterialAsset>::Create(material, setDefault); }
	private:
		Ref<Material> m_Material;
	};

	class MaterialTable : public RefCount
	{
	public:
		using MaterialMap = std::map<uint32_t, AssetHandle>;

	public:
		MaterialTable() = default;
		~MaterialTable() = default;

		bool HasMaterial(uint32_t index) const { return m_Materials.contains(index); }
		void SetMaterial(uint32_t index, AssetHandle material);
		void ClearMaterial(uint32_t index);

		AssetHandle GetMaterial(uint32_t index) const;

		MaterialMap& GetMaterials() { return m_Materials; }
		const MaterialMap& GetMaterials() const { return m_Materials; }

		void Clear();

	private:
		MaterialMap m_Materials;

	};

}
