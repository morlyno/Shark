#include "skpch.h"
#include "Shaders.h"

#include "Shark/Render/RendererCommand.h"

namespace Shark {

	Ref<Shaders> Shaders::Create(const std::string& filepath)
	{
		return RendererCommand::CreateShaders(filepath);
	}

	Ref<Shaders> Shaders::Create(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		return RendererCommand::CreateShaders(vertexshaderSrc, pixelshaderSrc);
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