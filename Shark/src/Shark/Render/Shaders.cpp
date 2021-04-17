#include "skpch.h"
#include "Shaders.h"

#include "Platform/DirectX11/DirectXShaders.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXShaders>::Allocate(filepath);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Shaders> Shaders::Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPI::API::DirectX11: return Ref<DirectXShaders>::Allocate(vertexshaderSrc, pixelshaderSrc);
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	void ShaderLib::Add(const std::string& name, Ref<Shaders> shaders)
	{
		SK_CORE_ASSERT(!Exits(name), "Shader already exists!");
		s_Shaders[name] = shaders;
	}

	void ShaderLib::Add(Ref<Shaders> shaders)
	{
		SK_CORE_ASSERT(!Exits(shaders->GetName()), "Shader already exists!");
		s_Shaders[shaders->GetName()] = shaders;
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
		return s_Shaders[name];
	}

	bool ShaderLib::Exits(const std::string& name)
	{
		return !(s_Shaders.find(name) == s_Shaders.end());
	}

}