#include "skpch.h"
#include "Shader.h"

#include "Shark/Render/Renderer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXShaderCompiler.h"

namespace Shark {

	Ref<Shader> Shader::Create()
	{
		switch (RendererAPI::GetCurrentAPI())
		{
			case RendererAPIType::None: SK_CORE_ASSERT(false, "No API Specified"); return nullptr;
			case RendererAPIType::DirectX11: return Ref<DirectXShader>::Create();
		}
		SK_CORE_ASSERT(false, "Unknown API");
		return nullptr;
	}

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath, bool forceCompile, bool disableOptimization)
	{
		auto shader = DirectXShaderCompiler::Compile(filepath, forceCompile, disableOptimization);
		if (!shader)
			return nullptr;

		SK_CORE_VERIFY(!Exists(shader->GetName()));
		m_ShaderMap[shader->GetName()] = shader;
		return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		SK_CORE_VERIFY(Exists(name), "The shader {} is not loaded!", name);
		return m_ShaderMap.at(name);
	}

	Ref<Shader> ShaderLibrary::TryGet(const std::string& name)
	{
		if (Exists(name))
			return m_ShaderMap.at(name);
		return nullptr;
	}

}