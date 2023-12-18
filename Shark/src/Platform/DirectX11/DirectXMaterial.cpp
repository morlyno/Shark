#include "skpch.h"
#include "DirectXMaterial.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <unordered_map>

namespace Shark {

	DirectXMaterial::DirectXMaterial(Ref<Shader> shader, const std::string& name)
		: m_Shader(shader.As<DirectXShader>()), m_Name(name)
	{
		Initialize();
	}

	DirectXMaterial::~DirectXMaterial()
	{
	}

	ShaderReflection::UpdateFrequencyType DirectXMaterial::GetUpdateFrequency(const std::string& name) const
	{
		if (!HasBuffer(name))
			return ShaderReflection::UpdateFrequencyType::None;

		auto& buffer = m_ConstantBuffers.at(name);
		return buffer.UpdateFrequency;
	}

	void DirectXMaterial::SetUpdateFrequency(const std::string& name, ShaderReflection::UpdateFrequencyType updateFrequency)
	{
		if (!HasBuffer(name))
			return;

		auto& buffer = m_ConstantBuffers.at(name);
		buffer.UpdateFrequency = updateFrequency;
	}

	void DirectXMaterial::Set(const std::string& name, bool val)
	{
		int intVal = val;
		SetBytes(name, Buffer::FromValue(intVal));
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
		return m_Resources.at(name).Sampler->GetSamplerID();
	}

	RenderID DirectXMaterial::GetSampler(const std::string& name, uint32_t index) const
	{
		std::string arrayName = fmt::format("{}_{}", name, index);
		SK_CORE_VERIFY(HasResourceName(arrayName));
		return m_Resources.at(arrayName).Sampler->GetSamplerID();
	}

	void DirectXMaterial::SetResource(const std::string& name, Ref<Texture2D> texture, Ref<Image2D> image, Ref<SamplerWrapper> sampler)
	{
		SK_CORE_VERIFY(HasResourceName(name));
		auto& resource = m_Resources.at(name);

		if (texture)
		{
			SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::Sampler2D);
			resource.Texture = texture.As<DirectXTexture2D>();
			resource.Image = texture->GetImage().As<DirectXImage2D>();
			resource.Sampler = texture->GetSampler().As<DirectXSamplerWrapper>();
			return;
		}

		if (image)
		{
			SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::Texture2D);
			resource.Texture = nullptr;
			resource.Image = image.As<DirectXImage2D>();
			resource.Sampler = nullptr;
			return;
		}

		if (sampler)
		{
			SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::Sampler);
			resource.Texture = nullptr;
			resource.Image = nullptr;
			resource.Sampler = sampler.As<DirectXSamplerWrapper>();
			return;
		}

		resource.Texture = nullptr;
		resource.Image = nullptr;
		resource.Sampler = nullptr;
	}

	void DirectXMaterial::SetResource(const std::string& name, uint32_t index, Ref<Texture2D> texture, Ref<Image2D> image, Ref<SamplerWrapper> sampler)
	{
		SetResource(fmt::format("{}_{}", name, index), texture, image, sampler);
	}

	void DirectXMaterial::SetBytes(const std::string& name, Buffer data)
	{
		SK_CORE_VERIFY(m_ConstantBufferMembers.contains(name));

		auto& member = m_ConstantBufferMembers.at(name);
		if (member.Parent->UpdateFrequency != ShaderReflection::UpdateFrequencyType::PerMaterial)
		{
			SK_CORE_WARN_TAG("Renderer", "Buffer ({}) with UpdateFrequency {} set in Material", name, ToString(member.Parent->UpdateFrequency));
		}

		member.UploadBufferRef.Write(data);
	}

	Buffer DirectXMaterial::GetBytes(const std::string& name) const
	{
		SK_CORE_VERIFY(m_ConstantBufferMembers.contains(name));
		auto& member = m_ConstantBufferMembers.at(name);
		return member.UploadBufferRef;
	}

	void DirectXMaterial::RT_UpdateBuffers()
	{
		for (auto& [name, cbData] : m_ConstantBuffers)
		{
			if (!cbData.Buffer)
			{
				auto& buffer = cbData.Buffer;
				buffer = Ref<DirectXConstantBuffer>::Create();
				buffer->SetSize(cbData.Size);
				buffer->SetBinding(cbData.Binding);
				buffer->RT_Invalidate();
			}

			cbData.Buffer->RT_UploadData(cbData.UploadBuffer.GetBuffer());
		}
	}

	void DirectXMaterial::Initialize()
	{
		const auto& reflectionData = m_Shader->GetReflectionData();

		for (const auto& [name, constantBuffer] : reflectionData.ConstantBuffers)
		{
			SK_CORE_VERIFY(!m_ConstantBuffers.contains(name));
			auto& cb = m_ConstantBuffers[name];
			cb.Size = constantBuffer.Size;
			cb.Binding = constantBuffer.Binding;
			cb.UpdateFrequency = constantBuffer.UpdateFrequency;
			//cb.Buffer = Ref<DirectXConstantBuffer>::Create(constantBuffer.Size, constantBuffer.Binding);
			cb.UploadBuffer.Allocate(constantBuffer.Size);
			cb.Stage = constantBuffer.Stage;

			for (const auto& member : constantBuffer.Members)
			{
				auto& memberData = m_ConstantBufferMembers[member.Name];
				memberData.Size = member.Size;
				memberData.Offset = member.Offset;
				memberData.UploadBufferRef = cb.UploadBuffer.GetBuffer().SubBuffer(member.Offset, member.Size);
				memberData.Parent = &cb;
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
