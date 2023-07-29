#include "skpch.h"
#include "DirectXMaterial.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <unordered_map>

namespace Shark {

	DirectXMaterial::DirectXMaterial(Ref<Shader> shader)
		: m_Shader(shader.As<DirectXShader>())
	{
		Initialize();
	}

	DirectXMaterial::~DirectXMaterial()
	{
	}

	Ref<Texture2D> DirectXMaterial::GetTexture(const std::string& name) const
	{
		SK_CORE_VERIFY(HasResourceName(name));
		return m_Resources.at(name).Texture;
	}

	Ref<Texture2D> DirectXMaterial::GetTexture(const std::string& name, uint32_t index) const
	{
		std::string arrayName = fmt::format("{}_{}", name, index);
		SK_CORE_VERIFY(HasResourceName(arrayName));
		return m_Resources.at(arrayName).Texture;
	}

	Ref<Image2D> DirectXMaterial::GetImage(const std::string& name) const
	{
		SK_CORE_VERIFY(HasResourceName(name));
		return m_Resources.at(name).Image;
	}

	Ref<Image2D> DirectXMaterial::GetImage(const std::string& name, uint32_t index) const
	{
		std::string arrayName = fmt::format("{}_{}", name, index);
		SK_CORE_VERIFY(HasResourceName(arrayName));
		return m_Resources.at(arrayName).Image;
	}

	RenderID DirectXMaterial::GetSampler(const std::string& name) const
	{
		SK_CORE_VERIFY(HasResourceName(name));
		return m_Resources.at(name).Sampler;
	}

	RenderID DirectXMaterial::GetSampler(const std::string& name, uint32_t index) const
	{
		std::string arrayName = fmt::format("{}_{}", name, index);
		SK_CORE_VERIFY(HasResourceName(arrayName));
		return m_Resources.at(arrayName).Sampler;
	}

	void DirectXMaterial::SetResource(const std::string& name, Ref<Texture2D> texture, Ref<Image2D> image, RenderID sampler)
	{
		SK_CORE_VERIFY(HasResourceName(name));
		auto& resource = m_Resources.at(name);

		static bool s_BreakWhenInputAndResourceTypeDontMatch = true;
#define SK_CORE_ASSERT_CONDITIONAL(cond, ...) if (cond) SK_CORE_ASSERT(__VA_ARGS__);

		if (texture)
		{
			SK_CORE_ASSERT_CONDITIONAL(s_BreakWhenInputAndResourceTypeDontMatch, resource.Type == ShaderReflection::ResourceType::Sampler2D);
			resource.Texture = texture.As<DirectXTexture2D>();
			resource.Image = texture->GetImage().As<DirectXImage2D>();
			resource.Sampler = resource.Texture->GetSamplerNative();
			return;
		}

		if (image)
		{
			SK_CORE_ASSERT_CONDITIONAL(s_BreakWhenInputAndResourceTypeDontMatch, resource.Type == ShaderReflection::ResourceType::Texture2D);
			resource.Texture = nullptr;
			resource.Image = image.As<DirectXImage2D>();
			resource.Sampler = nullptr;
			return;
		}

		if (sampler)
		{
			SK_CORE_ASSERT_CONDITIONAL(s_BreakWhenInputAndResourceTypeDontMatch, resource.Type == ShaderReflection::ResourceType::Sampler);
			resource.Texture = nullptr;
			resource.Image = nullptr;
			resource.Sampler = (ID3D11SamplerState*)sampler;
			return;
		}

		resource.Texture = nullptr;
		resource.Image = nullptr;
		resource.Sampler = nullptr;
	}

	void DirectXMaterial::SetResource(const std::string& name, uint32_t index, Ref<Texture2D> texture, Ref<Image2D> image, RenderID sampler)
	{
		SetResource(fmt::format("{}_{}", name, index), texture, image, sampler);
	}

	void DirectXMaterial::SetBytes(const std::string& name, Buffer data)
	{
		SK_CORE_VERIFY(m_ConstantBufferMembers.find(name) != m_ConstantBufferMembers.end());
		auto& member = m_ConstantBufferMembers.at(name);
		member.UploadBufferRef.Write(data);
	}

	Buffer DirectXMaterial::GetBytes(const std::string& name) const
	{
		SK_CORE_VERIFY(m_ConstantBufferMembers.find(name) != m_ConstantBufferMembers.end());
		auto& member = m_ConstantBufferMembers.at(name);
		return member.UploadBufferRef;
	}

	void DirectXMaterial::RT_UploadBuffers()
	{
		for (const auto& [name, cbData] : m_ConstantBuffers)
			cbData.Buffer->RT_UploadData(cbData.UploadBuffer.GetBuffer());
	}

	void DirectXMaterial::Initialize()
	{
		const auto& reflectionData = m_Shader->GetReflectionData();

		for (const auto& [name, constantBuffer] : reflectionData.ConstantBuffers)
		{
			SK_CORE_VERIFY(m_ConstantBuffers.find(name) == m_ConstantBuffers.end());
			auto& cb = m_ConstantBuffers[name];
			cb.Buffer = Ref<DirectXConstantBuffer>::Create(constantBuffer.Size, constantBuffer.Binding);
			cb.UploadBuffer.Allocate(constantBuffer.Size);
			cb.Stage = constantBuffer.Stage;

			for (const auto& member : constantBuffer.Members)
			{
				auto& memberData = m_ConstantBufferMembers[member.Name];
				memberData.Size = member.Size;
				memberData.Offset = member.Offset;
				memberData.UploadBufferRef = cb.UploadBuffer.GetBuffer().SubBuffer(member.Offset, member.Size);
			}
		}

		for (const auto& [name, resource] : reflectionData.Resources)
		{
			if (resource.ArraySize == 0)
			{
				auto& res = m_Resources[name];
				res.Type = resource.Type;
				res.Stage = resource.Stage;
				res.Binding = resource.Binding;
				continue;
			}

			for (uint32_t i = 0; i < resource.ArraySize; i++)
			{
				auto& res = m_Resources[fmt::format("{}_{}", name, i)];
				res.Type = resource.Type;
				res.Stage = resource.Stage;
				res.Binding = resource.Binding + i;
				continue;
			}
		}

	}

}
