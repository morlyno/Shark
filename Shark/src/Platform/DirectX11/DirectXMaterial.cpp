#include "skpch.h"
#include "DirectXMaterial.h"

#include "Shark/Utility/Utility.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <unordered_map>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif


namespace Shark {

	DirectXMaterial::DirectXMaterial(Ref<Shader> shader)
		: m_Shader(shader.As<DirectXShader>())
	{
		Reflect();
	}

	DirectXMaterial::~DirectXMaterial()
	{

	}

	void DirectXMaterial::SetTexture(const std::string& name, Ref<Texture2D> texture)
	{
		SK_CORE_ASSERT(Utility::Contains(m_ResourceMap, name));

		m_ResourceMap.at(name)->Set(0, texture);
	}

	void DirectXMaterial::SetTexture(const std::string& name, Ref<Texture2D> texture, uint32_t index)
	{
		SK_CORE_ASSERT(Utility::Contains(m_ResourceMap, name));

		m_ResourceMap.at(name)->Set(index, texture);
	}

	void DirectXMaterial::SetTextureArray(const std::string& name, Ref<Texture2DArray> textureArray)
	{
		SK_CORE_ASSERT(Utility::Contains(m_ResourceMap, name));

		Ref<Texture2DArray> arr = m_ResourceMap.at(name);
		SK_CORE_ASSERT(arr->Count() == textureArray->Count());
		for (uint32_t i = 0; i < textureArray->Count(); i++)
			arr->Set(i, textureArray->Get(i));
	}

	void DirectXMaterial::SetBytes(const std::string& name, byte* data, uint32_t size)
	{
		SK_CORE_ASSERT(Utility::Contains(m_VariableMap, name));

		const CBVar& var = m_VariableMap.at(name);
		Buffer& buffer = m_ConstantBufferData.at(var.BufferSlot);
		SK_CORE_ASSERT(size == var.Size, fmt::format("Invalid Data Size! Data Size is: {} but Size must be: {}", size, var.Size));
		buffer.Write(data, size, var.Offset);
	}

	byte* DirectXMaterial::GetBytes(const std::string& name) const
	{
		SK_CORE_ASSERT(Utility::Contains(m_VariableMap, name));

		const CBVar& var = m_VariableMap.at(name);
		const Buffer& buffer = m_ConstantBufferData.at(var.BufferSlot);

		return buffer.Data + var.Offset;

	}

	void DirectXMaterial::Reflect()
	{
		m_ConstnatBufferSet = Ref<DirectXConstantBufferSet>::Create();

		const auto& shaderBinarys = m_Shader->GetShaderBinarys();
		
		// VertexShader
		{
			const auto& vtxBinary = shaderBinarys.at(ShaderStage::Vertex);
			ID3D11ShaderReflection* reflection;
			SK_CHECK(D3DReflect(vtxBinary.data(), vtxBinary.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));


			D3D11_SHADER_DESC desc;
			reflection->GetDesc(&desc);

			for (uint32_t i = 0; i < desc.ConstantBuffers; i++)
			{
				ID3D11ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByIndex(i);

				D3D11_SHADER_BUFFER_DESC bufferDesc;
				constantBuffer->GetDesc(&bufferDesc);

				D3D11_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDescByName(bufferDesc.Name, &bindDesc);

				SK_CORE_ASSERT(bindDesc.BindCount == 1);
				m_ConstnatBufferSet->Create(bufferDesc.Size, bindDesc.BindPoint);
				m_ConstantBufferData[bindDesc.BindPoint].Allocate(bufferDesc.Size);

				for (uint32_t j = 0; j < bufferDesc.Variables; j++)
				{
					ID3D11ShaderReflectionVariable* variable = constantBuffer->GetVariableByIndex(j);
					D3D11_SHADER_VARIABLE_DESC variableDesc;
					variable->GetDesc(&variableDesc);

					std::string variableName = fmt::format("{}.{}", bufferDesc.Name, variableDesc.Name);
					m_VariableMap[variableName] = { bindDesc.BindPoint, variableDesc.StartOffset, variableDesc.Size };
				}
			}
		}

		// PixelShader
		{
			const auto& pxlBinary = shaderBinarys.at(ShaderStage::Pixel);
			ID3D11ShaderReflection* reflection;
			SK_CHECK(D3DReflect(pxlBinary.data(), pxlBinary.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));

			D3D11_SHADER_DESC desc;
			reflection->GetDesc(&desc);

			for (uint32_t i = 0; i < desc.BoundResources; i++)
			{
				D3D11_SHADER_INPUT_BIND_DESC bindDesc;
				reflection->GetResourceBindingDesc(i, &bindDesc);

				m_ResourceMap[bindDesc.Name] = Ref<DirectXTexture2DArray>::Create(bindDesc.BindCount, bindDesc.BindPoint);
			}
		}

	}

}
