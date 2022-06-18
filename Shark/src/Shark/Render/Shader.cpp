#include "skpch.h"
#include "Shader.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXShader.h"

namespace Shark {

	Ref<Shader> Shader::Create(const std::filesystem::path& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXShader>::Create(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath)
	{
		auto shaders = Shader::Create(filepath);
		Add(shaders);
		return shaders;
	}

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath, const std::string name)
	{
		auto shaders = Shader::Create(filepath);
		Add(shaders, name);
		return shaders;
	}

	void ShaderLibrary::Add(Ref<Shader> shader)
	{
		Add(shader, shader->GetFileName());
	}

	void ShaderLibrary::Add(Ref<Shader> shader, const std::string& name)
	{
		SK_CORE_ASSERT(!Exists(name));
		m_Shaders[name] = shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		SK_CORE_ASSERT(Exists(name));
		return m_Shaders[name];
	}

	Ref<Shader> ShaderLibrary::TryGet(const std::string& name)
	{
		const auto&& i = m_Shaders.find(name);
		if (i != m_Shaders.end())
			return i->second;
		return nullptr;
	}

	Ref<Shader> ShaderLibrary::Remove(const std::string& name)
	{
		auto shader = TryGet(name);
		if (!shader)
			return nullptr;
		m_Shaders.erase(name);
		return shader;
	}

	Ref<Shader> ShaderLibrary::Remove(Ref<Shader> shader)
	{
		auto i = std::find_if(m_Shaders.cbegin(), m_Shaders.cend(), [shader](const auto& other)
		{
			return shader == other.second;
		});

		if (i == m_Shaders.end())
			return nullptr;
		auto temp = i->second;
		m_Shaders.erase(i);
		return temp;
	}

	bool ShaderLibrary::Exists(const std::string& name)
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}

	bool ShaderLibrary::Exists(Ref<Shader> shader)
	{
		auto i = std::find_if(m_Shaders.cbegin(), m_Shaders.cend(), [shader](const auto& other)
		{
			return shader == other.second;
		});
		return i != m_Shaders.end();
	}

	void ShaderLibrary::Clear()
	{
		m_Shaders.clear();
	}

}