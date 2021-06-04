#include "skpch.h"
#include "DirectXMaterial.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11shader.h>
#include <d3dcompiler.h>

#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	enum class Stage
	{
		None = 0,
		Vertex, Pixel
	};

	namespace Utils {
		
		static const char* GetCacheDirectory()
		{
			return "assets/Cache/Shaders/DirectX/";
		}

		static Stage GetStageFromExtension(const std::string& extension)
		{
			if (extension == ".vert")
				return Stage::Vertex;
			if (extension == ".pixl")
				return Stage::Pixel;

			SK_CORE_ASSERT(false);
			return Stage::None;
		}

		static std::string ReadFile(const std::filesystem::path& filepath)
		{
			std::string result;
			std::ifstream in(filepath, std::ios::in | std::ios::binary);
			SK_CORE_ASSERT(in);
			if (in)
			{
				in.seekg(0, std::ios::end);
				result.resize(in.tellg());
				in.seekg(0, std::ios::beg);
				in.read(result.data(), result.size());
			}
			return std::move(result);
		}

		static DataType D3DDataTypeToSharkDataType(D3D_SHADER_VARIABLE_TYPE d3dtype)
		{
			switch (d3dtype)
			{
				case D3D_SVT_VOID:          return DataType::Void;
				case D3D_SVT_BOOL:          return DataType::Bool;
				case D3D_SVT_INT:           return DataType::Int;
				case D3D_SVT_FLOAT:         return DataType::Float;
				case D3D_SVT_STRING:        return DataType::String;
				case D3D_SVT_TEXTURE:       return DataType::Texture;
				case D3D_SVT_TEXTURE1D:     return DataType::Texture1D;
				case D3D_SVT_TEXTURE2D:     return DataType::Texture2D;
				case D3D_SVT_TEXTURE3D:     return DataType::Texture3D;
				case D3D_SVT_TEXTURECUBE:   return DataType::TextureCube;
			}
			SK_CORE_ASSERT(false);
			return DataType::None;
		}

		static DataType DataTypeFromTextureDimension(D3D_SRV_DIMENSION dimension)
		{
			switch (dimension)
			{
				case D3D_SRV_DIMENSION_TEXTURE1D:     return DataType::Texture1D;
				case D3D_SRV_DIMENSION_TEXTURE2D:     return DataType::Texture2D;
				case D3D_SRV_DIMENSION_TEXTURE3D:     return DataType::Texture3D;
				case D3D_SRV_DIMENSION_TEXTURECUBE:   return DataType::TextureCube;
			}
			SK_CORE_ASSERT(false);
			return DataType::None;
		}

		static bool ShouldBeIgnored(const std::string& bufferName)
		{
			if (bufferName == "SceneData")
				return true;
			return false;
		}

	}

	DirectXMaterial::DirectXMaterial(const Ref<Shaders>& shader, const std::string& name)
		: m_Shaders(shader), m_Name(name)
	{
		Reflect();
	}

	DirectXMaterial::~DirectXMaterial()
	{
	}

	void DirectXMaterial::Set(const std::string& name, const void* data, uint32_t size)
	{
#if SK_ENABLE_ASSERT
		if (m_Values.find(name) == m_Values.end())
		{
			SK_CORE_ERROR("Name {0} not found!", name);
			SK_CORE_INFO("Possible Name:");
			for (auto&& [name, val] : m_Values)
				SK_CORE_INFO(" - {0}", name);
			SK_CORE_ASSERT(false);
		}
#endif
		auto& val = m_Values[name];
		SK_CORE_ASSERT(val.Size == size);
		auto& tempBuffer = m_ConstantBuffers[val.BufferIndex].TempBuffer;
		memcpy(tempBuffer.data() + val.Offset, data, val.Size);
	}

	void DirectXMaterial::Set(const std::string& name, const Ref<Texture2D>& texture)
	{
		SK_CORE_ASSERT(m_Textures.find(name) != m_Textures.end(), "Name not found");
		auto& resource = m_Textures[name];
		resource.Texture = texture;
	}

	void DirectXMaterial::Bind()
	{
		m_Shaders->Bind();
		for (auto& cb : m_ConstantBuffers)
		{
			cb.ConstantBuffer->Set(cb.TempBuffer.data());
			cb.ConstantBuffer->Bind();
		}
		for (auto&& [key, resource] : m_Textures)
			if (resource.Texture)
				resource.Texture->Bind(resource.BindPoint);
	}

	void DirectXMaterial::Reflect()
	{
		std::filesystem::path cacheDir = Utils::GetCacheDirectory();
		const std::string& fileName = m_Shaders->GetFileName();
		

		for (auto&& file : std::filesystem::directory_iterator(cacheDir))
		{
			auto&& path = file.path();
			auto&& filename = path.filename().string();

			if (filename.find(fileName) != std::string::npos)
			{
				auto&& extension = path.extension().string();
				Stage stage = Utils::GetStageFromExtension(extension);

				switch (stage)
				{
					case Stage::Vertex:
					{
						std::string file = Utils::ReadFile(path);
						
						ID3D11ShaderReflection* reflection;
						SK_CHECK(D3DReflect(file.data(), file.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));

						D3D11_SHADER_DESC shaderDesc;
						SK_CHECK(reflection->GetDesc(&shaderDesc));

						for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; i++)
						{
							ID3D11ShaderReflectionConstantBuffer* constbuffer = reflection->GetConstantBufferByIndex(i);
							SK_CORE_ASSERT(constbuffer);
							D3D11_SHADER_BUFFER_DESC bufferDesc;
							SK_CHECK(constbuffer->GetDesc(&bufferDesc));

							if (Utils::ShouldBeIgnored(bufferDesc.Name))
								continue;

							D3D11_SHADER_INPUT_BIND_DESC inputDesc;
							SK_CHECK(reflection->GetResourceBindingDesc(i, &inputDesc));

							SK_CORE_ASSERT(inputDesc.BindCount == 1);
							SK_CORE_ASSERT(bufferDesc.Type == D3D11_CT_CBUFFER);

							ConstBuffer buffer;
							buffer.TempBuffer.resize(bufferDesc.Size);
							buffer.ConstantBuffer = ConstantBuffer::Create(bufferDesc.Size, inputDesc.BindPoint);
							buffer.DebugName = bufferDesc.Name;
							m_ConstantBuffers.emplace_back(buffer);

							for (uint32_t j = 0; j < bufferDesc.Variables; j++)
							{
								ID3D11ShaderReflectionVariable* variable = constbuffer->GetVariableByIndex(j);
								SK_CORE_ASSERT(variable);
								D3D11_SHADER_VARIABLE_DESC variableDesc;
								SK_CHECK(variable->GetDesc(&variableDesc));
								ID3D11ShaderReflectionType* type = variable->GetType();
								D3D11_SHADER_TYPE_DESC typeDesc;
								SK_CHECK(type->GetDesc(&typeDesc));
								

								BufferValue val;
								val.Size = variableDesc.Size;
								val.Offset = variableDesc.StartOffset;
								val.BufferIndex = m_ConstantBuffers.size() - 1;
								SK_CORE_ASSERT(m_Values.find(variableDesc.Name) == m_Values.end());
								m_Values[variableDesc.Name] = val;

								SK_CORE_ASSERT(typeDesc.Members == 0);
								SK_CORE_ASSERT(typeDesc.Offset == 0);
								auto& resourceDesc = m_Descriptor.Resources[variableDesc.Name];
								resourceDesc.Type = Utils::D3DDataTypeToSharkDataType(typeDesc.Type);
								resourceDesc.Rows = typeDesc.Rows;
								resourceDesc.Collums = typeDesc.Columns;
								resourceDesc.Data = m_ConstantBuffers[m_Values[variableDesc.Name].BufferIndex].TempBuffer.data() + variableDesc.StartOffset;
							}

						}


						break;
					}
					case Stage::Pixel:
					{
						std::string file = Utils::ReadFile(path);

						ID3D11ShaderReflection* reflection;
						SK_CHECK(D3DReflect(file.data(), file.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));

						D3D11_SHADER_DESC shaderDesc;
						SK_CHECK(reflection->GetDesc(&shaderDesc));

						for (uint32_t i = 0; i < shaderDesc.BoundResources; i++)
						{
							D3D11_SHADER_INPUT_BIND_DESC inputDesc;
							reflection->GetResourceBindingDesc(i, &inputDesc);

							if (inputDesc.Type == D3D_SIT_TEXTURE)
							{
								Resource resource;
								resource.BindPoint = inputDesc.BindPoint;
								m_Textures[inputDesc.Name] = resource;

								auto& resouceDesc = m_Descriptor.Resources[inputDesc.Name];
								resouceDesc.Type = Utils::DataTypeFromTextureDimension(inputDesc.Dimension);
								resouceDesc.Data = &m_Textures[inputDesc.Name].Texture;
							}

						}

						break;
					}
				}

			}

		}

	}

}
