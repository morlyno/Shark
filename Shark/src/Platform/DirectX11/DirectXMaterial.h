#pragma once

#include "Shark/Render/Material.h"

#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"
#include "Platform/DirectX11/DirectXTexture.h"

#include "Shark/Core/Buffer.h"

#include <d3d11.h>

namespace Shark {

	class DirectXMaterial : public Material
	{
	public:
		DirectXMaterial(Ref<Shader> shader);
		virtual ~DirectXMaterial();

		virtual Ref<Shader> GetShader() const override { return m_Shader; }
		virtual ShaderReflection::UpdateFrequencyType GetUpdateFrequency(const std::string& name) const override;
		virtual void SetUpdateFrequency(const std::string& name, ShaderReflection::UpdateFrequencyType updateFrequency) override;

		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture) override { SetResource(name, texture, nullptr, nullptr); }
		virtual void SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index) override { SetResource(name, index, texture, nullptr, nullptr); }

		virtual void SetImage(const std::string& name, Ref<Image2D> image) override { SetResource(name, nullptr, image, nullptr); }
		virtual void SetImage(const std::string& name, Ref<Image2D> image, uint32_t index) override { SetResource(name, index, nullptr, image, nullptr); }

		virtual void SetSampler(const std::string& name, Ref<SamplerWrapper> sampler) override { SetResource(name, nullptr, nullptr, sampler); }
		virtual void SetSampler(const std::string& name, Ref<SamplerWrapper> sampler, uint32_t index) override { SetResource(name, index, nullptr, nullptr, sampler); }

		virtual void SetFloat(const std::string& name, float val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetFloat2(const std::string& name, const glm::vec2& val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetFloat3(const std::string& name, const glm::vec3& val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetFloat4(const std::string& name, const glm::vec4& val) override { SetBytes(name, Buffer::FromValue(val)); }

		virtual void SetInt(const std::string& name, int val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetInt2(const std::string& name, const glm::ivec2& val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetInt3(const std::string& name, const glm::ivec3& val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetInt4(const std::string& name, const glm::ivec4& val) override { SetBytes(name, Buffer::FromValue(val)); }

		virtual void SetBool(const std::string& name, bool val) override { SetBytes(name, Buffer::FromValue(val)); }

		virtual void SetMat3(const std::string& name, const glm::mat3& val) override { SetBytes(name, Buffer::FromValue(val)); }
		virtual void SetMat4(const std::string& name, const glm::mat4& val) override { SetBytes(name, Buffer::FromValue(val)); }

		virtual Ref<Texture2D> GetTexture(const std::string& name) const override;
		virtual Ref<Texture2D> GetTexture(const std::string& name, uint32_t index) const override;

		virtual Ref<Image2D> GetImage(const std::string& name) const override;
		virtual Ref<Image2D> GetImage(const std::string& name, uint32_t index) const override;

		virtual RenderID GetSampler(const std::string& name) const override;
		virtual RenderID GetSampler(const std::string& name, uint32_t index) const override;

		virtual float GetFloat(const std::string& name) const override { return GetBytes(name).Value<float>(); }
		virtual const glm::vec2& GetFloat2(const std::string& name) const override { return GetBytes(name).Value<glm::vec2>(); }
		virtual const glm::vec3& GetFloat3(const std::string& name) const override { return GetBytes(name).Value<glm::vec3>(); }
		virtual const glm::vec4& GetFloat4(const std::string& name) const override { return GetBytes(name).Value<glm::vec4>(); }

		virtual int GetInt(const std::string& name, int val) override { return GetBytes(name).Value<int>(); }
		virtual const glm::ivec2& GetInt2(const std::string& name) const override { return GetBytes(name).Value<glm::ivec2>(); }
		virtual const glm::ivec3& GetInt3(const std::string& name) const override { return GetBytes(name).Value<glm::ivec3>(); }
		virtual const glm::ivec4& GetInt4(const std::string& name) const override { return GetBytes(name).Value<glm::ivec4>(); }

		virtual bool GetBool(const std::string& name) const override { return GetBytes(name).Value<bool>(); }

		virtual const glm::mat3& GetMat3(const std::string& name) const override { return GetBytes(name).Value<glm::mat3>(); }
		virtual const glm::mat4& GetMat4(const std::string& name) const override { return GetBytes(name).Value<glm::mat4>(); }

	private:
		void SetResource(const std::string& name, Ref<Texture2D> texture, Ref<Image2D> image, Ref<SamplerWrapper> sampler);
		void SetResource(const std::string& name, uint32_t index, Ref<Texture2D> texture, Ref<Image2D> image, Ref<SamplerWrapper> sampler);

		void SetBytes(const std::string& name, Buffer data);
		Buffer GetBytes(const std::string& name) const;

	private:
		bool HasResourceName(const std::string& name) const { return m_Resources.contains(name); }
		bool HasBuffer(const std::string& name) const { return m_ConstantBuffers.contains(name); }

		void RT_UpdateDirtyBuffers();

	private:
		void Initialize();

	private:
		Ref<DirectXShader> m_Shader;

		struct ConstantBufferData
		{
			uint32_t Size = 0;
			uint32_t Binding = 0;
			ShaderReflection::UpdateFrequencyType UpdateFrequency = ShaderReflection::UpdateFrequencyType::None;
			Ref<DirectXConstantBuffer> Buffer;
			ScopedBuffer UploadBuffer;
			ShaderReflection::ShaderStage Stage = ShaderReflection::ShaderStage::None;
			bool Dirty = false;
		};

		struct CBMember
		{
			uint32_t Offset = 0;
			uint32_t Size = 0;
			Buffer UploadBufferRef;
			ConstantBufferData* Parent = nullptr;
		};

		std::unordered_map<std::string, ConstantBufferData> m_ConstantBuffers;
		std::unordered_map<std::string, CBMember> m_ConstantBufferMembers;

		struct Resource
		{
			ShaderReflection::ResourceType Type = ShaderReflection::ResourceType::None;
			ShaderReflection::ShaderStage Stage = ShaderReflection::ShaderStage::None;
			uint32_t Binding;

			Ref<DirectXTexture2D> Texture;
			Ref<DirectXImage2D> Image;
			Ref<DirectXSamplerWrapper> Sampler;
		};

		std::unordered_map<std::string, Resource> m_Resources;

		friend class DirectXRenderer;
	};

}
