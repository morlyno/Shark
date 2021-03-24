#include "skpch.h"
#include "DirectXShaders.h"
#include "Shark/Render/Buffers.h"

#include <d3dcompiler.h>
#include <fstream>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	enum class ShaderType
	{
		None = 0,
		VertexShader, PixelShader
	};

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

	static std::string ReadFile(const std::string& filepath)
	{
		std::string result;
		std::ifstream in(filepath);
		SK_CORE_ASSERT(in, "File not found! Filepath: " + filepath);
		if (in)
		{
			in.seekg(0u, std::ios::end);
			result.resize((size_t)in.tellg());
			in.seekg(0u, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		return std::move(result);
	}

	static ShaderType StringToShaderEnum(const std::string& typestr)
	{
		if (typestr == "Vertex")
			return ShaderType::VertexShader;
		if (typestr == "Pixel")
			return ShaderType::PixelShader;
		SK_CORE_ASSERT(false, "could not detect shader type");
		return ShaderType::None;
	}

	DirectXShaders::DirectXShaders(const std::string& filepath, APIContext apicontext)
		: m_APIContext(apicontext)
	{
		std::string src = ReadFile(filepath);
		std::string VertexShader;
		std::string PixelShader;

		size_t ls = filepath.find_last_of("/\\");
		size_t en = filepath.find('.', ls);
		m_Name = filepath.substr(ls + 1, en - ls - 1);

		const char* typeToken = "#type";
		size_t TokenLength = strlen(typeToken);

		size_t offset = 0;
		while (true)
		{
			size_t tokenPos = src.find(typeToken, offset);
			if (tokenPos != std::string::npos)
			{
				size_t typePos = tokenPos + TokenLength + 1;
				size_t eol = src.find_first_of("\r\n", typePos);
				ShaderType shaderType = StringToShaderEnum(src.substr(typePos, eol - typePos));
				size_t end = src.find(typeToken, eol);
				// TODO: detect w/ string not w/ enum
				if (shaderType == ShaderType::VertexShader)
					VertexShader = src.substr(eol, end - eol);
				else if (shaderType == ShaderType::PixelShader)
					PixelShader = src.substr(eol, end - eol);
				offset = end;
				continue;
			}
			break;
		}

		Init(VertexShader, PixelShader);
	}

	DirectXShaders::DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc, APIContext apicontext)
		: m_APIContext(apicontext)
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
		ID3DBlob* ErrorMsg = nullptr;
		ID3DBlob* blob = nullptr;


		// Vertex Shader

		if (D3DCompile(vertexshaderSrc.c_str(), vertexshaderSrc.size(), nullptr, nullptr, nullptr, "main", "vs_4_0", 0u, 0u, &blob, &ErrorMsg) != S_OK)
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT(false, "Shader Compile Failed: " + msg);
			ErrorMsg->Release(); ErrorMsg = nullptr;
		}
		m_APIContext.Device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_VertexShader.shader);

		SK_CHECK(D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&m_VertexShader.reflection));

		D3D11_SHADER_DESC vsdesc;
		SK_CHECK(m_VertexShader.reflection->GetDesc(&vsdesc));

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputElements;
		m_InputElements.reserve(vsdesc.InputParameters);
		for (UINT i = 0; i < vsdesc.InputParameters; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			SK_CHECK(m_VertexShader.reflection->GetInputParameterDesc(i, &paramDesc));

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

		SK_CHECK(m_APIContext.Device->CreateInputLayout(m_InputElements.data(), (UINT)m_InputElements.size(), blob->GetBufferPointer(), blob->GetBufferSize(), &m_InputLayout));

		blob->Release(); blob = nullptr;

		for (UINT i = 0; i < vsdesc.ConstantBuffers; ++i)
		{
			ID3D11ShaderReflectionConstantBuffer* cbuff = m_VertexShader.reflection->GetConstantBufferByIndex(i);
			D3D11_SHADER_BUFFER_DESC sb;
			SK_CHECK(cbuff->GetDesc(&sb));

			ConstBuffer& buffer = m_VertexShader.constBuffers[sb.Name];

			buffer.size = sb.Size;
			buffer.slot = i;

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sb.Size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0u;
			bd.StructureByteStride = 0u;

			SK_CHECK(m_APIContext.Device->CreateBuffer(&bd, nullptr, &buffer.buffer));
		}

		// Pixel Shader

		if (D3DCompile(pixelshaderSrc.c_str(), pixelshaderSrc.size(), nullptr, nullptr, nullptr, "main", "ps_4_0", 0u, 0u, &blob, &ErrorMsg) != S_OK)
		{
			std::string msg = reinterpret_cast<const char*>(ErrorMsg->GetBufferPointer());
			SK_CORE_ASSERT(false, "Shader Compile Failed: " + msg);
			ErrorMsg->Release(); ErrorMsg = nullptr;
		}
		SK_CHECK(m_APIContext.Device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &m_PixelShader.shader));

		SK_CHECK(D3DReflect(blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D11ShaderReflection), (void**)&m_PixelShader.reflection));

		D3D11_SHADER_DESC psdesc;
		SK_CHECK(m_PixelShader.reflection->GetDesc(&psdesc));

		for (UINT i = 0; i < psdesc.ConstantBuffers; ++i)
		{
			ID3D11ShaderReflectionConstantBuffer* cbuff = m_PixelShader.reflection->GetConstantBufferByIndex(i);
			D3D11_SHADER_BUFFER_DESC sb;
			SK_CHECK(cbuff->GetDesc(&sb));

			ConstBuffer& buffer = m_PixelShader.constBuffers[sb.Name];

			buffer.size = sb.Size;
			buffer.slot = i;

			D3D11_BUFFER_DESC bd = {};
			bd.ByteWidth = sb.Size;
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			bd.MiscFlags = 0u;
			bd.StructureByteStride = 0u;

			SK_CHECK(m_APIContext.Device->CreateBuffer(&bd, nullptr, &buffer.buffer));
		}

		blob->Release(); blob = nullptr;
	}

	void DirectXShaders::SetBuffer(const std::string& bufferName, const Buffer& data)
	{
		UploudBuffer(bufferName, data);
	}

	void DirectXShaders::UploudBuffer(const std::string& bufferName, const Buffer& data)
	{
		ConstBuffer& buffer = m_VertexShader.constBuffers.find(bufferName)->second;
		SK_CORE_ASSERT(data.Size() == buffer.size, "data size and buffer size are not equal, sizes are: data:{0}, buffer:{1}");
		D3D11_MAPPED_SUBRESOURCE ms;
		SK_CHECK(m_APIContext.Context->Map(buffer.buffer, 0u, D3D11_MAP_WRITE_DISCARD, 0u, &ms));
		data.CopyInto(ms.pData);
		m_APIContext.Context->Unmap(buffer.buffer, 0u);
		m_APIContext.Context->VSSetConstantBuffers(buffer.slot, 1u, &buffer.buffer);
	}

	void DirectXShaders::Bind()
	{
		m_APIContext.Context->VSSetShader(m_VertexShader.shader, nullptr, 0u);
		m_APIContext.Context->PSSetShader(m_PixelShader.shader, nullptr, 0u);
		m_APIContext.Context->IASetInputLayout(m_InputLayout);
	}

	void DirectXShaders::UnBind()
	{
		m_APIContext.Context->VSSetShader(nullptr, nullptr, 0u);
		m_APIContext.Context->PSSetShader(nullptr, nullptr, 0u);
		m_APIContext.Context->IASetInputLayout(nullptr);
	}

}