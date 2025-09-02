#include "skpch.h"
#include "Shader.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/ShaderCompiler/ShaderCompiler.h"

#include "Shark/Platform/DirectX11/DirectX11Shader.h"

namespace Shark {

	Ref<Shader> Shader::Create()
	{
		switch (Application::Get().GetDeviceManager()->GetGraphicsAPI())
		{
			case nvrhi::GraphicsAPI::D3D11:
				return Ref<DirectX11Shader>::Create();
		}

		SK_CORE_VERIFY(false, "Unkown graphics api");
		return nullptr;
	}

	Ref<Shader> Shader::Create(Ref<ShaderCompiler> compiler)
	{
		switch (Application::Get().GetDeviceManager()->GetGraphicsAPI())
		{
			case nvrhi::GraphicsAPI::D3D11:
				return Ref<DirectX11Shader>::Create(compiler);
		}

		SK_CORE_VERIFY(false, "Unkown graphics api");
		return nullptr;
	}

	Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath, bool forceCompile, bool disableOptimization)
	{
		auto compiler = ShaderCompiler::Load(filepath, { .Force = forceCompile, .Optimize = !disableOptimization });
		if (!compiler)
			return nullptr;

		auto shader = Shader::Create(compiler);

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