#include "skpch.h"
#include "Shaders.h"

#include "Shark/Render/RendererAPI.h"
#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::filesystem::path& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXShaders>::Create(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Shaders> ShaderLibrary::Load(const std::filesystem::path& filepath)
	{
		auto shaders = Shaders::Create(filepath);
		Add(shaders);
		return shaders;
	}

	Ref<Shaders> ShaderLibrary::Load(const std::filesystem::path& filepath, const std::string name)
	{
		auto shaders = Shaders::Create(filepath);
		Add(shaders, name);
		return shaders;
	}

	void ShaderLibrary::Add(Ref<Shaders> shader)
	{
		Add(shader, shader->GetFileName());
	}

	void ShaderLibrary::Add(Ref<Shaders> shader, const std::string& name)
	{
		SK_CORE_ASSERT(!Exists(name));
		m_Shaders[name] = shader;
	}

	Ref<Shaders> ShaderLibrary::Get(const std::string& name)
	{
		SK_CORE_ASSERT(Exists(name));
		return m_Shaders[name];
	}

	Ref<Shaders> ShaderLibrary::TryGet(const std::string& name)
	{
		const auto&& i = m_Shaders.find(name);
		if (i != m_Shaders.end())
			return i->second;
		return nullptr;
	}

	Ref<Shaders> ShaderLibrary::Remove(const std::string& name)
	{
		auto shader = TryGet(name);
		if (shader == nullptr)
			return nullptr;
		m_Shaders.erase(name);
		return shader;
	}

	Ref<Shaders> ShaderLibrary::Remove(Ref<Shaders> shader)
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

	bool ShaderLibrary::Exists(Ref<Shaders> shader)
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