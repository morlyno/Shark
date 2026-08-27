#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {
	class Material;
}

namespace Shark {

	class PBRMaterial : public Asset
	{
	public:
		static Ref<PBRMaterial> Create(const std::string& name, bool setDefault = true, bool bakeMaterial = true) { return Ref<PBRMaterial>::Create(name, setDefault, bakeMaterial); }

	public:
		void SetDefaults();
		void Bake();
		void MT_Bake();
		void Update(bool async = true);

		void SetName(const std::string& name);
		const std::string& GetName() const;
		Ref<Material> GetMaterial() const;

		AssetHandle GetAlbedoMap();
		void SetAlbedoMap(AssetHandle handle);
		void ClearAlbedoMap();
		glm::vec3& GetAlbedoColor();
		void SetAlbedoColor(const glm::vec3& color);

		AssetHandle GetNormalMap();
		void SetNormalMap(AssetHandle handle);
		void ClearNormalMap(bool resetUsing = false);
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
		PBRMaterial(const std::string& name, bool setDefault = true, bool bakeMaterial = true);
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

}
