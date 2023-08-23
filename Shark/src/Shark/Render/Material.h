#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"
#include "Shark/Render/Texture.h"
#include "Shark/Render/ShaderReflection.h"

namespace Shark {

	class Material : public RefCount
	{
	public:
		virtual ~Material() = default;

		virtual Ref<Shader> GetShader() const = 0;
		virtual ShaderReflection::UpdateFrequencyType GetUpdateFrequency(const std::string& name) const = 0;
		virtual void SetUpdateFrequency(const std::string& name, ShaderReflection::UpdateFrequencyType updateFrequency) = 0;

		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture) = 0;
		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index) = 0;

		virtual void SetImage(const std::string& name, Ref<Image2D> image) = 0;
		virtual void SetImage(const std::string& name, Ref<Image2D> image, uint32_t index) = 0;

		virtual void SetSampler(const std::string& name, Ref<SamplerWrapper> sampler) = 0;
		virtual void SetSampler(const std::string& name, Ref<SamplerWrapper> sampler, uint32_t index) = 0;

		virtual void SetFloat(const std::string& name, float val) = 0;
		virtual void SetFloat2(const std::string& name, const glm::vec2& vec2) = 0;
		virtual void SetFloat3(const std::string& name, const glm::vec3& vec3) = 0;
		virtual void SetFloat4(const std::string& name, const glm::vec4& vec4) = 0;

		virtual void SetInt(const std::string& name, int val) = 0;
		virtual void SetInt2(const std::string& name, const glm::ivec2& vec2) = 0;
		virtual void SetInt3(const std::string& name, const glm::ivec3& vec3) = 0;
		virtual void SetInt4(const std::string& name, const glm::ivec4& vec4) = 0;

		virtual void SetBool(const std::string& name, bool val) = 0;

		virtual void SetMat3(const std::string& name, const glm::mat3& mat3) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& mat4) = 0;

		virtual Ref<Texture2D> GetTexture(const std::string& name) const = 0;
		virtual Ref<Texture2D> GetTexture(const std::string& name, uint32_t index) const = 0;

		virtual Ref<Image2D> GetImage(const std::string& name) const = 0;
		virtual Ref<Image2D> GetImage(const std::string& name, uint32_t index) const = 0;

		virtual RenderID GetSampler(const std::string& name) const = 0;
		virtual RenderID GetSampler(const std::string& name, uint32_t index) const = 0;

		virtual float GetFloat(const std::string& name) const = 0;
		virtual const glm::vec2& GetFloat2(const std::string& name) const = 0;
		virtual const glm::vec3& GetFloat3(const std::string& name) const = 0;
		virtual const glm::vec4& GetFloat4(const std::string& name) const = 0;

		virtual int GetInt(const std::string& name, int val) = 0;
		virtual const glm::ivec2& GetInt2(const std::string& name) const = 0;
		virtual const glm::ivec3& GetInt3(const std::string& name) const = 0;
		virtual const glm::ivec4& GetInt4(const std::string& name) const = 0;

		virtual bool GetBool(const std::string& name) const = 0;

		virtual const glm::mat3& GetMat3(const std::string& name) const = 0;
		virtual const glm::mat4& GetMat4(const std::string& name) const = 0;

	public:
		static Ref<Material> Create(Ref<Shader> shader);
	};

	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset();
		MaterialAsset(Ref<Material> material)
			: m_Material(material)
		{}

		Ref<Material> GetMaterial() const { return m_Material; }
		void SetMaterial(Ref<Material> material) { m_Material = material; }

		bool IsDirty() const { return m_Dirty; }
		void SetDirty(bool dirty) { m_Dirty = dirty; }

		const glm::vec3& GetAlbedoColor() const { return m_AlbedoColor; }
		AssetHandle GetAlbedoTexture() const { return m_AlbedoTexture; }
		bool UseAlbedo() const { return m_UseAlbedo; }

		void SetAlbedoColor(const glm::vec3& color) { m_AlbedoColor = color; m_Dirty = true; }
		void SetAlbedoTexture(AssetHandle handle) { m_AlbedoTexture = handle; m_Dirty = true; }
		void SetUseAlbedo(bool use) { m_UseAlbedo = use; m_Dirty = true; }

		float GetMetallic() const { return m_Metallic; }
		float GetRoughness() const { return m_Roughness; }
		float GetAO() const { return m_AO; }

		void SetMetallic(float metallic) { m_Metallic = metallic; m_Dirty = true; }
		void SetRoughness(float roughness) { m_Roughness = roughness; m_Dirty = true; }
		void SetAO(float ao) { m_AO = ao; m_Dirty = true; }

		void UpdateMaterial();
		void UpdateMaterialIfDirty() { if (IsDirty()) UpdateMaterial(); }

	public:
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
		static AssetType GetStaticType() { return AssetType::Material; }

		static Ref<MaterialAsset> Create() { return Ref<MaterialAsset>::Create(); }

	private:
		Ref<Material> m_Material;

		bool m_Dirty = false;

		glm::vec3 m_AlbedoColor = glm::vec3(1.0f);
		AssetHandle m_AlbedoTexture;
		bool m_UseAlbedo = false;

		float m_Metallic = 0.0f;
		float m_Roughness = 0.0f;
		float m_AO = 0.0f;

		friend class MaterialSerializer;
		friend class MeshSourceSerializer;
	};

	class MaterialTable : public RefCount
	{
	public:
		using MaterialMap = std::map<uint32_t, Ref<MaterialAsset>>;

	public:
		MaterialTable() = default;
		~MaterialTable() = default;

		bool HasMaterial(uint32_t index) const
		{
			return m_MaterialAssets.find(index) != m_MaterialAssets.end();
		}

		Ref<MaterialAsset> GetMaterial(uint32_t index) const
		{
			SK_CORE_ASSERT(HasMaterial(index));
			return m_MaterialAssets.at(index);
		}

		void AddMaterial(uint32_t index, Ref<MaterialAsset> materialAsset)
		{
			SK_CORE_ASSERT(!HasMaterial(index));
			m_MaterialAssets[index] = materialAsset;
		}

		// Adds or Sets the Material at the given index. If Material is null Removes the Material at the given index
		void SetMaterial(uint32_t index, Ref<MaterialAsset> material)
		{
			if (!material)
			{
				RemoveMaterial(index);
				return;
			}

			m_MaterialAssets[index] = material;
		}

		void RemoveMaterial(uint32_t index)
		{
			m_MaterialAssets.erase(index);
		}

		MaterialMap::iterator begin() { return m_MaterialAssets.begin(); }
		MaterialMap::iterator end() { return m_MaterialAssets.end(); }

	private:
		MaterialMap m_MaterialAssets;

	};

}
