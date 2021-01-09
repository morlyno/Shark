#include "skpch.h"
#include "DirectXShaders.h"
#include "Shark/Render/Buffers.h"
#include "Shark/Render/RendererCommand.h"
#include "DirectXRendererAPI.h"
#include "Shark/Core/Application.h"

#include <d3dcompiler.h>

#define SK_GET_RENDERERAPI() static_cast<::Shark::DirectXRendererAPI&>(::Shark::RendererCommand::GetRendererAPI())


namespace Shark {

	//None = 0,
	//	Float, Float2, Float3, Float4,
	//	Int, Int2, Int3, Int4,

	static ShaderDataType DXGIFormatToShaderDataType(DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R32_FLOAT:             return ShaderDataType::Float;
			case DXGI_FORMAT_R32G32_FLOAT:          return ShaderDataType::Float2;
			case DXGI_FORMAT_R32G32B32_FLOAT:       return ShaderDataType::Float3;
			case DXGI_FORMAT_R32G32B32A32_FLOAT:    return ShaderDataType::Float4;
			case DXGI_FORMAT_R32_SINT:              return ShaderDataType::Int;
			case DXGI_FORMAT_R32G32_SINT:           return ShaderDataType::Int2;
			case DXGI_FORMAT_R32G32B32_SINT:        return ShaderDataType::Int3;
			case DXGI_FORMAT_R32G32B32A32_SINT:     return ShaderDataType::Int4;
		}
		SK_CORE_ASSERT(false, "Unkown format");
		return ShaderDataType::None;
	}

	DirectXShaders::DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		Init(vertexshaderSrc, pixelshaderSrc);
	}

	DirectXShaders::~DirectXShaders()
	{
		if (m_VertexShader.shader) { m_VertexShader.shader->Release(); m_VertexShader.shader = nullptr; }
		if (m_VertexShader.reflection) { m_VertexShader.reflection->Release(); m_VertexShader.reflection = nullptr; }
		for (auto& b : m_VertexShader.constBuffers)
			if (b.second.buffer) { b.second.buffer->Release(); b.second.buffer = nullptr; }
		if (m_PixelShader.shader) { m_PixelShader.shader->Release(); m_PixelShader.shader = nullptr; }
		if (m_PixelShader.reflection) { m_PixelShader.reflection->Release(); m_PixelShader.reflection = nullptr; }
		for (auto& b : m_PixelShader.constBuffers)
			if (b.second.buffer) { b.second.buffer->Release(); b.second.buffer = nullptr; }

		if (m_InputLayout) { m_InputLayout->Release(); m_InputLayout = nullptr; }
	}

	void DirectXShaders::Init(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc)
	{
		auto device = SK_GET_RENDERERAPI().GetDevice();

		ID3DBlob* ErrorMsg = nullptr;
		ID3DBlob* blob = nullptr;


		// Vertex Shader

		if (D3DCompile(vertexshaderSrc.c_str(), vertexshaderSrc.size(), nullptr, nullptr, nullptr, "main", "vs_4_0", 0u, 0u, &blob, &ErrorMsg) != S_OK)
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT(false, "Shader Compile Failed" + msg);
			ErrorMsg->Release(); ErrorMsg = nullptr;
		}
		device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_VertexShader.shader);

		D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&m_VertexShader.reflection);

		D3D11_SHADER_DESC vsdesc;
		m_VertexShader.reflection->GetDesc(&vsdesc);

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputElements;
		m_InputElements.reserve(vsdesc.InputParameters);
		for (UINT i = 0; i < vsdesc.InputParameters; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			m_VertexShader.reflection->GetInputParameterDesc(i, &paramDesc);

			D3D11_INPUT_ELEMENT_DESC elementDesc;
			elementDesc.SemanticName = paramDesc.SemanticName;
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			elementDesc.InstanceDataStepRate = 0;

			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			m_InputElements.push_back(elementDesc);
			m_VertexLayout.Add({ DXGIFormatToShaderDataType(elementDesc.Format), elementDesc.SemanticName });
		}
		m_VertexLayout.Init();

		device->CreateInputLayout(m_InputElements.data(), (UINT)m_InputElements.size(), blob->GetBufferPointer(), blob->GetBufferSize(), &m_InputLayout);

		blob->Release(); blob = nullptr;

		for (UINT i = 0; i < vsdesc.ConstantBuffers; ++i)
		{
			ID3D11ShaderReflectionConstantBuffer* cbuff = m_VertexShader.reflection->GetConstantBufferByIndex(i);
			D3D11_SHADER_BUFFER_DESC sb;
			cbuff->GetDesc(&sb);

			Buffer& buffer = m_VertexShader.constBuffers[sb.Name];

			buffer.size = sb.Size;
			buffer.slot = i;

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sb.Size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0u;
			bd.StructureByteStride = 0u;

			device->CreateBuffer(&bd, nullptr, &buffer.buffer);
		}

		// Pixel Shader

		if (D3DCompile(pixelshaderSrc.c_str(), pixelshaderSrc.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0u, 0u, &blob, &ErrorMsg) != S_OK)
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT(false, "Shader Compile Failed" + msg);
			ErrorMsg->Release(); ErrorMsg = nullptr;
		}
		device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_PixelShader.shader);

		D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&m_PixelShader.reflection);

		D3D11_SHADER_DESC psdesc;
		m_PixelShader.reflection->GetDesc(&psdesc);

		for (UINT i = 0; i < psdesc.ConstantBuffers; ++i)
		{
			ID3D11ShaderReflectionConstantBuffer* cbuff = m_PixelShader.reflection->GetConstantBufferByIndex(i);
			D3D11_SHADER_BUFFER_DESC sb;
			cbuff->GetDesc(&sb);

			Buffer& buffer = m_PixelShader.constBuffers[sb.Name];

			buffer.size = sb.Size;
			buffer.slot = i;

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sb.Size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0u;
			bd.StructureByteStride = 0u;

			device->CreateBuffer(&bd, nullptr, &buffer.buffer);
		}

		blob->Release(); blob = nullptr;
	}

	void DirectXShaders::UploudData(const std::string& bufferName, ShaderType type, void* data, uint32_t size)
	{
		auto context = SK_GET_RENDERERAPI().GetContext();

		if (type == ShaderType::PixelShader)
		{
			Buffer& buffer = m_PixelShader.constBuffers.find(bufferName)->second;
			D3D11_MAPPED_SUBRESOURCE ms;
			context->Map(buffer.buffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &ms);
			memcpy(ms.pData, data, size);
			context->Unmap(buffer.buffer, 0u);
			context->PSSetConstantBuffers(buffer.slot, 1u, &buffer.buffer);
		}
		else if (type == ShaderType::VertexShader)
		{
			Buffer& buffer = m_VertexShader.constBuffers.find(bufferName)->second;
			D3D11_MAPPED_SUBRESOURCE ms;
			context->Map(buffer.buffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &ms);
			memcpy(ms.pData, data, size);
			context->Unmap(buffer.buffer, 0u);
			context->VSSetConstantBuffers(buffer.slot, 1u, &buffer.buffer);
		}
	}

	void DirectXShaders::Bind()
	{
		auto context = SK_GET_RENDERERAPI().GetContext();
		context->VSSetShader(m_VertexShader.shader, nullptr, 0u);
		context->PSSetShader(m_PixelShader.shader, nullptr, 0u);
		context->IASetInputLayout(m_InputLayout);
	}

	void DirectXShaders::UnBind()
	{
		auto context = SK_GET_RENDERERAPI().GetContext();
		context->VSSetShader(nullptr, nullptr, 0u);
		context->PSSetShader(nullptr, nullptr, 0u);
		context->IASetInputLayout(nullptr);
	}

}