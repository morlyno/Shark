#pragma once

#include "Shark/Render/Material.h"

namespace Shark {

	class PBRMaterial : public Asset
	{
	public:
		static Ref<PBRMaterial> Create(const std::string& name, bool setDefault = true) { return Ref<PBRMaterial>::Create(name, setDefault); }

	public:
		void SetDefaults();
		void PrepareAndUpdate();

		const std::string& GetName() const { return m_Material->GetName(); }
		Ref<Material> GetMaterial() const { return m_Material; }

		AssetHandle GetAlbedoMap();
		void SetAlbedoMap(AssetHandle handle);
		void ClearAlbedoMap();
		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		AssetHandle GetNormalMap();
		void SetNormalMap(AssetHandle handle);
		void ClearNormalMap();
		bool IsUsingNormalMap();
		void SetUsingNormalMap(bool value);

		AssetHandle GetMetalnessMap();
		void SetMetalnessMap(AssetHandle handle);
		void ClearMetalnessMap();
		float& GetMetalness();
		void SetMetalness(float value);

		AssetHandle GetRoughnessMap();
		void SetRoughnessMap(AssetHandle handle);
		void ClearRoughnessMap();
		float& GetRoughness();
		void SetRoughness(float value);

	public:
		PBRMaterial(const std::string& name, bool setDefault = true);
		~PBRMaterial();

		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Material; }

	private:
		struct Uniforms
		{
			glm::vec3 Albedo;
			float Metalness;
			float Roughness;
			float AmbientOcclusion;
			bool UsingNormalMap;
			float P0;

			bool operator==(const Uniforms& other) const = default;
			bool operator!=(const Uniforms& other) const = default;
		};

	private:
		Ref<Material> m_Material;

		AssetHandle m_AlbedoMap;
		AssetHandle m_NormalMap;
		AssetHandle m_MetalnessMap;
		AssetHandle m_RoughnessMap;

		Uniforms m_Uniforms;
		Uniforms m_ActiveState;
	};

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
