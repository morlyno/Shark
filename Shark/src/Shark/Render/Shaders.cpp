#include "skpch.h"
#include "Shaders.h"
#include "RendererAPI.h"
#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXShaders>(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Shaders> Shaders::Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "RendererAPI not specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Create_Ref<DirectXShaders>(vertexshaderSrc, pixelshaderSrc);
		}
		SK_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	std::unordered_map<std::string, Ref<Shaders>> ShaderLib::m_Shaders;

	void ShaderLib::Add(const std::string& name, Ref<Shaders> shaders)
	{
		SK_CORE_ASSERT(!Exits(name), "Shader already exists!");
		m_Shaders[name] = shaders;
	}

	void ShaderLib::Add(Ref<Shaders> shaders)
	{
		SK_CORE_ASSERT(!Exits(shaders->GetName()), "Shader already exists!");
		m_Shaders[shaders->GetName()] = shaders;
	}

	Ref<Shaders> ShaderLib::Load(const std::string& name, const std::string& filepath)
	{
		Ref<Shaders> shader = Shaders::Create(filepath);
		Add(name, shader);
		return shader;
	}

	Ref<Shaders> ShaderLib::Load(const std::string& filepath)
	{
		Ref<Shaders> shader = Shaders::Create(filepath);
		Add(shader);
		return shader;
	}

	Ref<Shaders> ShaderLib::Get(const std::string& name)
	{
		return m_Shaders[name];
	}

	bool ShaderLib::Exits(const std::string& name)
	{
		return !(m_Shaders.find(name) == m_Shaders.end());
	}

}