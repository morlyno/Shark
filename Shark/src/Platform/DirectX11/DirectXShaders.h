#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	struct ConstBuffer
	{
		ID3D11Buffer* buffer;
		UINT size;
		UINT slot;
	};

	struct PixelShader
	{
		ID3D11PixelShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::unordered_map<std::string, ConstBuffer> constBuffers;
	};

	struct VertexShader
	{
		ID3D11VertexShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::unordered_map<std::string, ConstBuffer> constBuffers;
	};

	class DirectXShaders : public Shaders
	{
	public:
		DirectXShaders(const std::string& filepath, APIContext apicontext);
		DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc, APIContext apicontext);
		~DirectXShaders();

		void Init(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);

		virtual void SetBuffer(const std::string& bufferName, const Buffer& data) override;

		void UploudBuffer(const std::string& bufferName, const Buffer& data);

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		void Bind() override;
		void UnBind() override;

		virtual const std::string& GetName() override { return m_Name; }
	private:
		PixelShader m_PixelShader;
		VertexShader m_VertexShader;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		VertexLayout m_VertexLayout;

		std::string m_Name;

		APIContext m_APIContext;
	};

}