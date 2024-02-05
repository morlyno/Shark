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

	void DirectXMaterial::SetResource(const std::string& name, Ref<Texture2D> texture)
	{
		SK_CORE_VERIFY(HasResourceName(name));

		if (!texture)
		{
			ClearResource(name);
			return;
		}

		auto& resource = m_Resources.at(name);
		SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::Texture2D);
		resource.Texture = texture.As<DirectXTexture2D>();
		resource.TextureCube = nullptr;
		resource.Image = texture->GetImage().As<DirectXImage2D>();
		resource.Sampler = texture->GetSampler().As<DirectXSamplerWrapper>();
	}

	void DirectXMaterial::SetResource(const std::string& name, Ref<TextureCube> textureCube)
	{
		SK_CORE_VERIFY(HasResourceName(name));

		if (!textureCube)
		{
			ClearResource(name);
			return;
		}

		auto& resource = m_Resources.at(name);
		SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::TextureCube);
		resource.Texture = nullptr;
		resource.TextureCube = textureCube.As<DirectXTextureCube>();
		resource.Image = textureCube->GetImage().As<DirectXImage2D>();
		resource.Sampler = textureCube->GetSampler().As<DirectXSamplerWrapper>();
	}

	void DirectXMaterial::SetResource(const std::string& name, Ref<Image2D> image)
	{
		SK_CORE_VERIFY(HasResourceName(name));

		if (!image)
		{
			ClearResource(name);
			return;
		}

		auto& resource = m_Resources.at(name);
		SK_CORE_VERIFY((image->GetType() == ImageType::Texture     && resource.Type == ShaderReflection::ResourceType::Image2D) ||
					   (image->GetType() == ImageType::Atachment   && resource.Type == ShaderReflection::ResourceType::Image2D) ||
					   (image->GetType() == ImageType::TextureCube && resource.Type == ShaderReflection::ResourceType::ImageCube));

		resource.Texture = nullptr;
		resource.TextureCube = nullptr;
		resource.Image = image.As<DirectXImage2D>();
		resource.Sampler = nullptr;
	}

	void DirectXMaterial::SetResource(const std::string& name, Ref<SamplerWrapper> sampler)
	{
		SK_CORE_VERIFY(HasResourceName(name));

		if (!sampler)
		{
			ClearResource(name);
			return;
		}

		auto& resource = m_Resources.at(name);
		SK_CORE_VERIFY(resource.Type == ShaderReflection::ResourceType::Sampler);
		resource.Texture = nullptr;
		resource.TextureCube = nullptr;
		resource.Image = nullptr;
		resource.Sampler = sampler.As<DirectXSamplerWrapper>();
	}

	void DirectXMaterial::ClearResource(const std::string& name)
	{
		SK_CORE_VERIFY(HasResourceName(name));
		auto& resource = m_Resources.at(name);
		resource.Texture = nullptr;
		resource.TextureCube = nullptr;
		resource.Image = nullptr;
		resource.Sampler = nullptr;
	}

	void DirectXMaterial::SetBytes(const std::string& name, Buffer data)
	{
		SK_CORE_VERIFY(m_ConstantBufferMembers.contains(name));

		auto& member = m_ConstantBufferMembers.at(name);
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
			if (constantBuffer.UpdateFrequency != ShaderReflection::UpdateFrequencyType::PerMaterial)
				continue;

			SK_CORE_VERIFY(!m_ConstantBuffers.contains(name));
			auto& cb = m_ConstantBuffers[name];
			cb.Size = constantBuffer.Size;
			cb.Binding = constantBuffer.Binding;
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
			if (resource.UpdateFrequency != ShaderReflection::UpdateFrequencyType::PerMaterial)
				continue;

			if (resource.ArraySize == 0)
			{
				Resource& res = m_Resources[name];
				res.Type = resource.Type;
				res.Stage = resource.Stage;
				res.Binding = resource.Binding;
				res.SamplerBinding = resource.SamplerBinding;
				continue;
			}

			for (uint32_t i = 0; i < resource.ArraySize; i++)
			{
				Resource& res = m_Resources[fmt::format("{}_{}", name, i)];
				res.Type = resource.Type;
				res.Stage = resource.Stage;
				res.Binding = resource.Binding + i;
				res.SamplerBinding = resource.SamplerBinding;
				continue;
			}
		}

	}

}
