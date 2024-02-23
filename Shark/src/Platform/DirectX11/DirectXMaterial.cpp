#include "skpch.h"
#include "DirectXMaterial.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXRenderer.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <unordered_map>

namespace Shark {

	DirectXMaterial::DirectXMaterial(Ref<Shader> shader, const std::string& name)
		: m_Shader(shader), m_Name(name), m_ShaderInputManager(shader)
	{
		Renderer::AcknowledgeShaderDependency(shader, this);
		SetInputConstantBuffers();
	}

	DirectXMaterial::~DirectXMaterial()
	{
	}

	void DirectXMaterial::Prepare()
	{
		m_ShaderInputManager.Update();
	}

	bool DirectXMaterial::Validate() const
	{
		return m_ShaderInputManager.ValidateMaterialInputs();
	}

	void DirectXMaterial::Set(const std::string& name, Ref<Texture2D> texture)
	{
		m_ShaderInputManager.SetInput(name, texture);
	}

	void DirectXMaterial::Set(const std::string& name, Ref<TextureCube> textureCube)
	{
		m_ShaderInputManager.SetInput(name, textureCube);
	}

	void DirectXMaterial::Set(const std::string& name, Ref<Image2D> image)
	{
		m_ShaderInputManager.SetInput(name, image);
	}

	void DirectXMaterial::Set(const std::string& name, Ref<SamplerWrapper> sampler)
	{
		m_ShaderInputManager.SetInput(name, sampler);
	}

	Ref<Texture2D> DirectXMaterial::GetTexture(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<Texture2D>(name);
	}

	Ref<TextureCube> DirectXMaterial::GetTextureCube(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<TextureCube>(name);
	}

	Ref<Image2D> DirectXMaterial::GetImage(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<Image2D>(name);
	}

	Ref<SamplerWrapper> DirectXMaterial::GetSampler(const std::string& name) const
	{
		return m_ShaderInputManager.GetResource<SamplerWrapper>(name);
	}

	void DirectXMaterial::Set(const std::string& name, float val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::vec2& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::vec3& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::vec4& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, int val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::ivec2& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::ivec3& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::ivec4& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, bool val)
	{
		uint32_t intVal = val;
		SetMember(name, Buffer::FromValue(intVal));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::mat3& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	void DirectXMaterial::Set(const std::string& name, const glm::mat4& val)
	{
		SetMember(name, Buffer::FromValue(val));
	}

	float& DirectXMaterial::GetFloat(const std::string& name)
	{
		return Get<float>(name);
	}

	glm::vec2& DirectXMaterial::GetVec2(const std::string& name)
	{
		return Get<glm::vec2>(name);
	}

	glm::vec3& DirectXMaterial::GetVec3(const std::string& name)
	{
		return Get<glm::vec3>(name);
	}

	glm::vec4& DirectXMaterial::GetVec4(const std::string& name)
	{
		return Get<glm::vec4>(name);
	}

	int& DirectXMaterial::GetInt(const std::string& name)
	{
		return Get<int>(name);
	}

	glm::ivec2& DirectXMaterial::GetInt2(const std::string& name)
	{
		return Get<glm::ivec2>(name);
	}

	glm::ivec3& DirectXMaterial::GetInt3(const std::string& name)
	{
		return Get<glm::ivec3>(name);
	}

	glm::ivec4& DirectXMaterial::GetInt4(const std::string& name)
	{
		return Get<glm::ivec4>(name);
	}

	bool& DirectXMaterial::GetBool(const std::string& name)
	{
		return Get<bool>(name);
	}

	glm::mat3& DirectXMaterial::GetMat3(const std::string& name)
	{
		return Get<glm::mat3>(name);
	}

	glm::mat4& DirectXMaterial::GetMat4(const std::string& name)
	{
		return Get<glm::mat4>(name);
	}

	void DirectXMaterial::RT_UploadBuffers()
	{
		for (const auto& constantBuffer : m_ConstantBuffers)
		{
			constantBuffer->RT_Upload();
		}
	}

	void DirectXMaterial::SetMember(const std::string& name, Buffer data)
	{
		SK_CORE_ASSERT(m_Shader->HasMember(name));
		if (!m_Shader->HasMember(name))
			return;

		const auto& memberInfo = m_Shader->GetMemberInfo(name);
		const auto& resourceInfo = m_Shader->GetMembersResourceInfo(name);
		Ref<ConstantBuffer> input = m_ShaderInputManager.GetResource<ConstantBuffer>(resourceInfo.Name);
		SK_CORE_VERIFY(input);

		SK_CORE_ASSERT(data.Size == memberInfo.Size);
		if (data.Size != memberInfo.Size)
			return;

		Buffer uploadBuffer = input->GetUploadBuffer();
		uploadBuffer.Write(data, memberInfo.Size, memberInfo.Offset);
	}

	Buffer DirectXMaterial::GetMember(const std::string& name)
	{
		if (!m_Shader->HasMember(name))
			return {};

		const auto& memberInfo = m_Shader->GetMemberInfo(name);
		const auto& resourceInfo = m_Shader->GetMembersResourceInfo(name);
		Ref<ConstantBuffer> resource = m_ShaderInputManager.GetResource<ConstantBuffer>(resourceInfo.Name);
		SK_CORE_VERIFY(resource);
		return resource->GetUploadBuffer().SubBuffer(memberInfo.Offset, memberInfo.Size);
	}

	void DirectXMaterial::SetInputConstantBuffers()
	{
		const auto& reflectionData = m_Shader->GetReflectionData();
		if (!reflectionData.Resources.contains(0))
			return;

		const auto& set0Resources = reflectionData.Resources.at(0);
		for (const auto& [binding, resource] : set0Resources)
		{
			if (resource.Type != ShaderReflection::ResourceType::ConstantBuffer)
				continue;

			Ref<ConstantBuffer> constantBuffer = ConstantBuffer::Create(resource.StructSize);
			m_ShaderInputManager.SetInput(resource.Name, constantBuffer);
			m_ConstantBuffers.push_back(constantBuffer);
		}
	}

}
