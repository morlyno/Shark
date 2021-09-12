#include "skpch.h"
#include "DirectXShaders.h"
#include "Shark/Utility/Utility.h"
#include "Shark/Core/Timer.h"
#include "Shark/Utility/FileSystem.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"
#include "Shark/Core/Application.h"

#include <d3dcompiler.h>


#ifdef SK_ENABLE_ASSERT
#define SK_CHECK(call) if(HRESULT hr = (call); FAILED(hr)) { SK_CORE_ERROR("0x{0:x}", hr); SK_DEBUG_BREAK(); }
#else
#define SK_CHECK(call) call
#endif

namespace Shark {

	namespace Utils {

		static const char* D3DConstBufferTypeAsString(D3D_CBUFFER_TYPE d3dcbuffertype)
		{
			switch ((int)d3dcbuffertype)
			{
				case D3D_CT_CBUFFER: return SK_STRINGIFY(D3D_CT_CBUFFER);
				case D3D_CT_TBUFFER: return SK_STRINGIFY(D3D_CT_TBUFFER);
			}
			return "Unkonw";
		}

		static const char* D3DTypeAsString(D3D_SHADER_INPUT_TYPE d3dtype)
		{
			switch ((int)d3dtype)
			{
				case D3D_SIT_CBUFFER: return SK_STRINGIFY(D3D_SIT_CBUFFER);
				case D3D_SIT_TBUFFER: return SK_STRINGIFY(D3D_SIT_TBUFFER);
				case D3D_SIT_TEXTURE: return SK_STRINGIFY(D3D_SIT_TEXTURE);
				case D3D_SIT_SAMPLER: return SK_STRINGIFY(D3D_SIT_SAMPLER);
			}
			return "Unkown";
		}

		static const char* D3DReturnTypeAsString(D3D_RESOURCE_RETURN_TYPE d3dreturntype)
		{
			switch ((int)d3dreturntype)
			{
				case D3D_RETURN_TYPE_UNORM:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_UNORM);
				case D3D_RETURN_TYPE_SNORM:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_SNORM);
				case D3D_RETURN_TYPE_SINT:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_SINT);
				case D3D_RETURN_TYPE_UINT:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_UINT);
				case D3D_RETURN_TYPE_FLOAT:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_FLOAT);
				case D3D_RETURN_TYPE_MIXED:	     return SK_STRINGIFY(D3D11_RETURN_TYPE_MIXED);
				case D3D_RETURN_TYPE_DOUBLE:	 return SK_STRINGIFY(D3D11_RETURN_TYPE_DOUBLE);
				case D3D_RETURN_TYPE_CONTINUED:  return SK_STRINGIFY(D3D11_RETURN_TYPE_CONTINUED);
			}
			return "Unkown";
		}

		static const char* D3DDimensionAsString(D3D_SRV_DIMENSION d3ddimension)
		{
			switch (d3ddimension)
			{
				case D3D_SRV_DIMENSION_UNKNOWN:			  return SK_STRINGIFY(D3D_SRV_DIMENSION_UNKNOWN);
				case D3D_SRV_DIMENSION_BUFFER:			  return SK_STRINGIFY(D3D_SRV_DIMENSION_BUFFER);
				case D3D_SRV_DIMENSION_TEXTURE1D:		  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE1D);
				case D3D_SRV_DIMENSION_TEXTURE1DARRAY:	  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE1DARRAY);
				case D3D_SRV_DIMENSION_TEXTURE2D:		  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE2D);
				case D3D_SRV_DIMENSION_TEXTURE2DARRAY:	  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE2DARRAY);
				case D3D_SRV_DIMENSION_TEXTURE2DMS:		  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE2DMS);
				case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE2DMSARRAY);
				case D3D_SRV_DIMENSION_TEXTURE3D:		  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURE3D);
				case D3D_SRV_DIMENSION_TEXTURECUBE:		  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURECUBE);
				case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:  return SK_STRINGIFY(D3D_SRV_DIMENSION_TEXTURECUBEARRAY);
				case D3D_SRV_DIMENSION_BUFFEREX:          return SK_STRINGIFY(D3D_SRV_DIMENSION_BUFFEREX);
			}
			return "Unkown";
		}

		static VertexDataType DXGIFormatToVertexDataType(DXGI_FORMAT format)
		{
			switch (format)
			{
				case DXGI_FORMAT_R32_FLOAT:           return VertexDataType::Float;
				case DXGI_FORMAT_R32G32_FLOAT:        return VertexDataType::Float2;
				case DXGI_FORMAT_R32G32B32_FLOAT:     return VertexDataType::Float3;
				case DXGI_FORMAT_R32G32B32A32_FLOAT:  return VertexDataType::Float4;
				case DXGI_FORMAT_R32_SINT:            return VertexDataType::Int;
				case DXGI_FORMAT_R32G32_SINT:         return VertexDataType::Int2;
				case DXGI_FORMAT_R32G32B32_SINT:      return VertexDataType::Int3;
				case DXGI_FORMAT_R32G32B32A32_SINT:   return VertexDataType::Int4;
			}
			SK_CORE_ASSERT(false, "Unkown format");
			return VertexDataType::None;
		}

		static Shader StringToEnum(const std::string& typestr)
		{
			std::string str = Utility::ToLower(typestr);
			if (str == "vertex")
				return Shader::Vertex;
			if (str == "pixel" || str == "fragment")
				return Shader::Pixel;
			SK_CORE_ASSERT(false, "could not detect shader type");
			return Shader::None;
		}

		static const char* StageToString(Shader stage)
		{
			switch (stage)
			{
				case Shader::Vertex: return "Vertex";
				case Shader::Pixel: return "Pixel";
			}
			SK_CORE_ASSERT(false);
			return nullptr;
		}

		static std::string ExtractVersion(std::string& src)
		{
			size_t versiontokenpos = src.find("#version");
			if (versiontokenpos == std::string::npos)
				return std::string{};

			size_t versionpos = src.find_first_not_of(" ", versiontokenpos + strlen("#version"));
			size_t end = src.find_first_of("\n ", versionpos);
			std::string version = src.substr(versionpos, end - versionpos);
			src.erase(versiontokenpos, end - versiontokenpos);
			return version;
		}

		static std::filesystem::path CacheDirectory()
		{
			auto& proj = Application::Get().GetProject();
			return proj.GetCacheDirectory() / "Shaders/DirectX";
		}

		static const char* FileExtension(Shader stage)
		{
			switch (stage)
			{
				case Shader::Vertex:  return ".cached_directx.vert";
				case Shader::Pixel:   return ".cached_directx.pixl";
			}
			SK_CORE_ASSERT(false, "Unkown Shader stage");
			return nullptr;
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			const auto cacheDirectory = Utils::CacheDirectory();
			if (!FileSystem::Exists(cacheDirectory))
				FileSystem::CreateDirectorys(cacheDirectory);
		}

	}

	DirectXShaders::DirectXShaders(const std::filesystem::path& filepath)
	{
		m_FilePath = filepath;
		m_FileName = filepath.filename().replace_extension().string();

		std::string file = ReadFile(filepath);
		auto shaderSources = PreProzess(file);
		
		{
			Timer timer;
			CompileOrGetCached(shaderSources);

			SK_CORE_TRACE("Shader Compile tock {0} ms [File: {1}]", timer.Stop(), filepath);
		}

		CreateShaders();
		CreateInputlayout(m_ShaderBinarys[Shader::Vertex]);
		//Reflect();

	}

	DirectXShaders::~DirectXShaders()
	{
		Release();
	}

	void DirectXShaders::Release()
	{
		if (m_PixelShader)
		{
			m_PixelShader->Release();
			m_PixelShader = nullptr;
		}

		if (m_VertexShader)
		{
			m_VertexShader->Release();
			m_VertexShader = nullptr;
		}

		if (m_InputLayout)
		{
			m_InputLayout->Release();
			m_InputLayout = nullptr;
		}

	}

	bool DirectXShaders::ReCompile()
	{
		std::string file = ReadFile(m_FilePath);
		auto shaderSources = PreProzess(file);

		{
			Timer timer;
			if (!TryReCompile(shaderSources))
				return false;

			SK_CORE_TRACE("Shader ReCompile tock {0} ms [File: {1}]", timer.Stop(), m_FilePath);
		}
		Release();

		CreateShaders();
		CreateInputlayout(m_ShaderBinarys[Shader::Vertex]);
		//Reflect();

		return true;
	}

	Ref<ConstantBuffer> DirectXShaders::CreateConstantBuffer(const std::string& name)
	{
		ID3D11ShaderReflection* reflection;
		auto&& binary = m_ShaderBinarys[Shader::Vertex];
		SK_CHECK(D3DReflect(binary.data(), binary.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));
		ID3D11ShaderReflectionConstantBuffer* constantBuffer = reflection->GetConstantBufferByName(name.c_str());
		SK_CORE_ASSERT(constantBuffer);
		D3D11_SHADER_BUFFER_DESC bufferDesc;
		constantBuffer->GetDesc(&bufferDesc);
		D3D11_SHADER_INPUT_BIND_DESC inputDesc;
		reflection->GetResourceBindingDescByName(name.c_str(), &inputDesc);

		reflection->Release();

		return ConstantBuffer::Create(bufferDesc.Size, inputDesc.BindPoint);
	}

	void DirectXShaders::Bind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();
		ctx->VSSetShader(m_VertexShader, nullptr, 0u);
		ctx->PSSetShader(m_PixelShader, nullptr, 0u);
		ctx->IASetInputLayout(m_InputLayout);
	}

	void DirectXShaders::UnBind()
	{
		auto* ctx = DirectXRendererAPI::GetContext();
		ctx->VSSetShader(nullptr, nullptr, 0u);
		ctx->PSSetShader(nullptr, nullptr, 0u);
		ctx->IASetInputLayout(nullptr);
	}

	std::string DirectXShaders::ReadFile(const std::filesystem::path& filepath)
	{
		std::string result;
		std::ifstream in(filepath, std::ios::in, std::ios::binary);
		SK_CORE_ASSERT(in, fmt::format("File not found! Filepath: {}", filepath));
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

	std::unordered_map<Shader, std::string> DirectXShaders::PreProzess(const std::string& file)
	{
		std::unordered_map<Shader, std::string> shaders;

		const char* TypeToken = "#type";
		size_t TokenLenght = strlen(TypeToken);

		size_t offset = 0;
		while (offset != std::string::npos)
		{
			size_t token_pos = file.find(TypeToken, offset);
			if (token_pos == std::string::npos)
				break;

			token_pos = file.find_first_not_of(" ", token_pos + TokenLenght);
			size_t type_end = file.find_first_of("\n ", token_pos);
			Shader type = Utils::StringToEnum(file.substr(token_pos, type_end - token_pos));
			offset = type_end;

			size_t src_end = file.find(TypeToken, offset);
			std::string src = file.substr(offset, src_end - offset);
			offset = src_end;

			SK_CORE_ASSERT(shaders.find(type) == shaders.end());
			shaders[type] = src;

		}

		return std::move(shaders);

	}

	bool DirectXShaders::TryReCompile(std::unordered_map<Shader, std::string>& shaderSources)
	{
		std::filesystem::path cacheDirectory = Utils::CacheDirectory();
		Utils::CreateCacheDirectoryIfNeeded();

		std::unordered_map<Shader, std::vector<byte>> tempShaderBinarys;
		for (auto&& [stage, src] : shaderSources)
		{
			D3D_SHADER_MACRO define[1];
			define[0].Name = "DIRECTX";
			define[0].Definition = "1";

			auto version = Utils::ExtractVersion(src);

			ID3DBlob* shaderBinary = nullptr;
			ID3DBlob* errorMsg = nullptr;
			if (FAILED(D3DCompile(src.c_str(), src.size(), m_FileName.c_str(), nullptr, nullptr, "main", version.c_str(), 0, 0, &shaderBinary, &errorMsg)))
			{
				SK_CORE_ERROR("Shader Compile Failed");
				SK_CORE_ERROR(" - File: {0}", Utils::StageToString(stage));
				SK_CORE_ERROR(" - Error Msg: {0}", (char*)errorMsg->GetBufferPointer());
				return false;
			}

			auto& binary = tempShaderBinarys[stage];
			binary.resize(shaderBinary->GetBufferSize());
			memcpy(binary.data(), shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());
		}

		for (auto&& [stage, binary] : tempShaderBinarys)
		{
			std::filesystem::path cacheFile = cacheDirectory / (m_FileName + Utils::FileExtension(stage));
			std::ofstream out(cacheFile, std::ios::out | std::ios::binary);
			SK_CORE_ASSERT(out);
			if (out)
			{
				out.write((const char*)(binary.data()), binary.size());
				out.close();
			}
		}
		m_ShaderBinarys = std::move(tempShaderBinarys);
		return true;
	}

	void DirectXShaders::CompileOrGetCached(std::unordered_map<Shader, std::string>& shaderSources)
	{
		std::filesystem::path cacheDirectory = Utils::CacheDirectory();
		Utils::CreateCacheDirectoryIfNeeded();

		m_ShaderBinarys.clear();
		for (auto&& [stage, src] : shaderSources)
		{
			std::filesystem::path cacheFile = cacheDirectory / (m_FileName + Utils::FileExtension(stage));

			if (std::filesystem::exists(cacheFile))
			{
				std::ifstream in(cacheFile, std::ios::in | std::ios::binary);
				if (in)
				{
					auto& binary = m_ShaderBinarys[stage];

					in.seekg(0, std::ios::end);
					binary.resize(in.tellg());
					in.seekg(0, std::ios::beg);

					in.read((char*)binary.data(), binary.size());
					in.close();
				}
				else
				{
					SK_CORE_ASSERT(false);
				}

			}
			else
			{
				D3D_SHADER_MACRO define[1];
				define[0].Name = "DIRECTX";
				define[0].Definition = "1";

				auto version = Utils::ExtractVersion(src);

				ID3DBlob* shaderBinary = nullptr;
				ID3DBlob* errorMsg = nullptr;
				if (FAILED(D3DCompile(src.c_str(), src.size(), m_FileName.c_str(), nullptr, nullptr, "main", version.c_str(), 0, 0, &shaderBinary, &errorMsg)))
				{
					SK_CORE_ERROR("Shader Compile Failed");
					SK_CORE_ERROR(" - File: {0}", Utils::StageToString(stage));
					SK_CORE_ERROR(" - Error Msg: {0}", (char*)errorMsg->GetBufferPointer());
					SK_CORE_ASSERT(false);
				}

				auto& binary = m_ShaderBinarys[stage];
				binary.resize(shaderBinary->GetBufferSize());
				memcpy(binary.data(), shaderBinary->GetBufferPointer(), shaderBinary->GetBufferSize());

				std::ofstream out(cacheFile, std::ios::out | std::ios::binary);
				SK_CORE_ASSERT(out);
				if (out)
				{
					out.write((const char*)(binary.data()), binary.size());
					out.close();
				}
			}

		}

	}

	void DirectXShaders::Reflect()
	{
		for (auto&& [stage, binary] : m_ShaderBinarys)
		{
			ID3D11ShaderReflection* reflection;
			SK_CHECK(D3DReflect((void*)binary.data(), (UINT)binary.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));

			D3D11_SHADER_DESC desc;
			reflection->GetDesc(&desc);

			SK_CORE_TRACE("{0} Shader Reflection", Utils::StageToString(stage));
			SK_CORE_TRACE(" - Buffers: {0}", desc.ConstantBuffers);

			for (uint32_t i = 0; i < desc.ConstantBuffers; i++)
			{
				auto buffer = reflection->GetConstantBufferByIndex(i);
				D3D11_SHADER_BUFFER_DESC bufferdesc;
				buffer->GetDesc(&bufferdesc);

				SK_CORE_TRACE("   - Name: {0}", bufferdesc.Name);
				SK_CORE_TRACE("     - Type: {0}", Utils::D3DConstBufferTypeAsString(bufferdesc.Type));
				SK_CORE_TRACE("     - Size: {0}", bufferdesc.Size);
				SK_CORE_TRACE("     - Flags: 0x{0:x}", bufferdesc.uFlags);
				SK_CORE_TRACE("     - Variables: {0}", bufferdesc.Variables);

				for (uint32_t i = 0; i < bufferdesc.Variables; i++)
				{
					auto variable = buffer->GetVariableByIndex(i);
					D3D11_SHADER_VARIABLE_DESC variabledesc;
					variable->GetDesc(&variabledesc);
					
					SK_CORE_TRACE("       - Name: {0}", variabledesc.Name);
					SK_CORE_TRACE("         - Offset: {0}", variabledesc.StartOffset);
					SK_CORE_TRACE("         - Size: {0}", variabledesc.Size);
					SK_CORE_TRACE("         - Flags: 0x{0:x}", variabledesc.uFlags);
					SK_CORE_TRACE("         - StartTexture: {0}", variabledesc.StartTexture);
					SK_CORE_TRACE("         - TextureSize: {0}", variabledesc.TextureSize);
					SK_CORE_TRACE("         - StartSampler: {0}", variabledesc.StartSampler);
					SK_CORE_TRACE("         - SamplerSize: {0}", variabledesc.SamplerSize);
				}

			}

			SK_CORE_TRACE(" - Resources: {0}", desc.BoundResources);

			for (uint32_t i = 0; i < desc.BoundResources; i++)
			{
				D3D11_SHADER_INPUT_BIND_DESC ibd;
				reflection->GetResourceBindingDesc(i, &ibd);
				SK_CORE_TRACE("   - Name: {0}", ibd.Name);
				SK_CORE_TRACE("     - Type: {0}", Utils::D3DTypeAsString(ibd.Type));
				SK_CORE_TRACE("     - BindPoint: {0}", ibd.BindPoint);
				SK_CORE_TRACE("     - BindCount: {0}", ibd.BindCount);
				SK_CORE_TRACE("     - Flags: 0x{0:x}", ibd.uFlags);
				SK_CORE_TRACE("     - ReturnType: {0}", Utils::D3DReturnTypeAsString(ibd.ReturnType));
				SK_CORE_TRACE("     - Dimension: {0}", Utils::D3DDimensionAsString(ibd.Dimension));
				SK_CORE_TRACE("     - NumSampler: {0}", ibd.NumSamples);
			}
			reflection->Release();
		}
	}

	void DirectXShaders::CreateInputlayout(const std::vector<byte>& vtx_src)
	{
		ID3D11ShaderReflection* reflection;

		SK_CHECK(D3DReflect((void*)vtx_src.data(), (UINT)vtx_src.size(), __uuidof(ID3D11ShaderReflection), (void**)&reflection));

		D3D11_SHADER_DESC vsdesc;
		SK_CHECK(reflection->GetDesc(&vsdesc));

		std::vector<D3D11_INPUT_ELEMENT_DESC> m_InputElements;
		m_InputElements.reserve(vsdesc.InputParameters);
		for (UINT i = 0; i < vsdesc.InputParameters; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			SK_CHECK(reflection->GetInputParameterDesc(i, &paramDesc));

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
			m_VertexLayout.Add({ Utils::DXGIFormatToVertexDataType(elementDesc.Format), elementDesc.SemanticName });
		}
		m_VertexLayout.Init();

		auto dev = DirectXRendererAPI::GetDevice();
		SK_CHECK(dev->CreateInputLayout(m_InputElements.data(), (UINT)m_InputElements.size(), (void*)vtx_src.data(), (UINT)vtx_src.size(), &m_InputLayout));
		reflection->Release();

	}

	void DirectXShaders::CreateShaders()
	{
		auto* dev = DirectXRendererAPI::GetDevice();

		SK_CORE_ASSERT(m_VertexShader == nullptr);
		SK_CORE_ASSERT(m_PixelShader == nullptr);

		for (auto&& [stage, binary] : m_ShaderBinarys)
		{
			switch (stage)
			{
				case Shader::Vertex:
					SK_CHECK(dev->CreateVertexShader((void*)binary.data(), binary.size(), nullptr, &m_VertexShader));
					break;
				case Shader::Pixel:
					SK_CHECK(dev->CreatePixelShader((void*)binary.data(), binary.size(), nullptr, &m_PixelShader));
					break;
				default:
					SK_CORE_ASSERT(false);
					break;
			}
		}
	}

}