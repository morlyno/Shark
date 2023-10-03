#include "skpch.h"
#include "DirectXShader.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Timer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/Utils/String.h"
#include "Shark/Debug/Profiler.h"

#include "Platform/DirectX11/DirectXRenderer.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXShaderCompiler.h"

#include <d3dcompiler.h>

namespace Shark {

	DirectXShader::DirectXShader()
	{
	}

	DirectXShader::~DirectXShader()
	{
		Release();
	}

	void DirectXShader::Release()
	{
		Renderer::SubmitResourceFree([pixel = m_PixelShader, vertex = m_VertexShader]()
		{
			if (pixel)
				pixel->Release();
			if (vertex)
				vertex->Release();
		});

		m_PixelShader = nullptr;
		m_VertexShader = nullptr;
	}

	bool DirectXShader::Reload(bool forceCompile, bool disableOptimization)
	{
		Ref<DirectXShaderCompiler> compiler = Ref<DirectXShaderCompiler>::Create(m_FilePath, disableOptimization);
		if (!compiler->Reload(forceCompile))
			return false;

		Release();
		LoadShader(compiler->GetShaderBinary());
		return true;
	}

	void DirectXShader::LoadShader(const std::unordered_map<ShaderUtils::ShaderStage::Type, std::vector<byte>>& shaderBinary)
	{
		auto renderer = DirectXRenderer::Get();
		auto device = renderer->GetDevice();

		m_ShaderBinary = shaderBinary;

		Ref<DirectXShader> instance = this;
		Renderer::Submit([instance]()
		{
			auto renderer = DirectXRenderer::Get();
			auto device = renderer->GetDevice();

			for (const auto& [stage, binary] : instance->m_ShaderBinary)
			{
				switch (stage)
				{
					case ShaderUtils::ShaderStage::Vertex:
						SK_DX11_CALL(device->CreateVertexShader(binary.data(), binary.size(), nullptr, &instance->m_VertexShader));
						D3D_SET_OBJECT_NAME_A(instance->m_VertexShader, instance->m_Name.c_str());
						break;
					case ShaderUtils::ShaderStage::Pixel:
						SK_DX11_CALL(device->CreatePixelShader(binary.data(), binary.size(), nullptr, &instance->m_PixelShader));
						D3D_SET_OBJECT_NAME_A(instance->m_PixelShader, instance->m_Name.c_str());
						break;
					default:
						SK_CORE_VERIFY(false, "Unkown Shader Stage");
						break;
				}
			}
		});
	}

}