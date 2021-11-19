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
		SK_CORE_ASSERT(arr->GetCount() == textureArray->GetCount());
		for (uint32_t i = 0; i < textureArray->GetCount(); i++)
			arr->Set(i, textureArray->Get(i));
	}

	void DirectXMaterial::SetFloat(const std::string& name, float val)
	{

	}

	void DirectXMaterial::SetFloat2(const std::string& name, const DirectX::XMFLOAT2& vec2)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetFloat3(const std::string& name, const DirectX::XMFLOAT3& vec3)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetFloat4(const std::string& name, const DirectX::XMFLOAT4& vec4)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetInt(const std::string& name, int val)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetInt2(const std::string& name, const DirectX::XMINT2& vec2)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetInt3(const std::string& name, const DirectX::XMINT3& vec3)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetInt4(const std::string& name, const DirectX::XMINT4& vec4)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetBool(const std::string& name, bool val)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetMat3(const std::string& name, const DirectX::XMFLOAT3X3& mat3)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetMat4(const std::string& name, const DirectX::XMFLOAT4X4& mat4)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetMat4(const std::string& name, const DirectX::XMMATRIX& mat4)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	void DirectXMaterial::SetBytes(const std::string& name, byte* data, uint32_t size)
	{
		SK_CORE_ASSERT(Utility::Contains(m_VariableMap, name));
		SK_CORE_ASSERT(false, "Not Implemented");

		//const CBVar& var = m_VariableMap.at(name);
		//Buffer& buffer = m_ConstantBuffers.at(var.BufferSlot);
		//SK_CORE_ASSERT(size == var.Size, fmt::format("Invalid Data Size! Data Size is: {} but Size must be: {}", size, var.Size));
		//buffer.Write(data, size, var.Offset);
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
				//m_ConstantBuffers[bindDesc.BindPoint].Allocate(bufferDesc.Size);

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
