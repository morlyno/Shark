#pragma once

#include "Shark/Core/Buffer.h"
#include "Shark/Render/Material.h"

#include "Platform/DirectX11/ShaderInputManager.h"

#include <d3d11.h>

namespace Shark {

	class DirectXMaterial : public Material
	{
	public:
		DirectXMaterial(Ref<Shader> shader, const std::string& name);
		virtual ~DirectXMaterial();

		virtual void Prepare() override;
		virtual bool Validate() const override;

		virtual Ref<Shader> GetShader() const override { return m_Shader; }
		virtual const std::string& GetName() const override { return m_Name; }
		virtual void SetName(const std::string& name) override { m_Name = name; }

		virtual void Set(const std::string& name, Ref<Texture2D> texture) override;
		virtual void Set(const std::string& name, Ref<TextureCube> textureCube) override;
		virtual void Set(const std::string& name, Ref<Image2D> image) override;
		virtual void Set(const std::string& name, Ref<SamplerWrapper> sampler) override;

		virtual Ref<Texture2D> GetTexture(const std::string& name) const override;
		virtual Ref<TextureCube> GetTextureCube(const std::string& name) const override;
		virtual Ref<Image2D> GetImage(const std::string& name) const override;
		virtual Ref<SamplerWrapper> GetSampler(const std::string& name) const override;

		virtual void Set(const std::string& name, float val) override;
		virtual void Set(const std::string& name, const glm::vec2& val) override;
		virtual void Set(const std::string& name, const glm::vec3& val) override;
		virtual void Set(const std::string& name, const glm::vec4& val) override;

		virtual void Set(const std::string& name, int val) override;
		virtual void Set(const std::string& name, const glm::ivec2& val) override;
		virtual void Set(const std::string& name, const glm::ivec3& val) override;
		virtual void Set(const std::string& name, const glm::ivec4& val) override;

		virtual void Set(const std::string& name, bool val) override;

		virtual void Set(const std::string& name, const glm::mat3& val) override;
		virtual void Set(const std::string& name, const glm::mat4& val) override;

		virtual float&     GetFloat(const std::string& name) override;
		virtual glm::vec2& GetVec2(const std::string& name) override;
		virtual glm::vec3& GetVec3(const std::string& name) override;
		virtual glm::vec4& GetVec4(const std::string& name) override;

		virtual int&        GetInt(const std::string& name) override;
		virtual glm::ivec2& GetInt2(const std::string& name) override;
		virtual glm::ivec3& GetInt3(const std::string& name) override;
		virtual glm::ivec4& GetInt4(const std::string& name) override;

		virtual bool& GetBool(const std::string& name) override;

		virtual glm::mat3& GetMat3(const std::string& name) override;
		virtual glm::mat4& GetMat4(const std::string& name) override;

		template<typename T>
		T& Get(const std::string& name)
		{
			return GetMember(name).Value<T>();
		}

		void RT_UploadBuffers();
		const std::vector<BoundResource>& GetBoundResources() const { return m_ShaderInputManager.GetBoundResources(); }

	private:
		void SetMember(const std::string& name, Buffer data);
		Buffer GetMember(const std::string& name);

		void SetInputConstantBuffers();

	private:
		std::string m_Name;
		Ref<Shader> m_Shader;
		ShaderInputManager m_ShaderInputManager;

		std::vector<Ref<ConstantBuffer>> m_ConstantBuffers;

		friend class DirectXRenderer;
	};

}
