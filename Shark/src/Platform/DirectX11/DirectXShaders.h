#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"

#include "Platform/DirectX11/DirectXRendererAPI.h"

#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	struct ConstBuffer
	{
		ID3D11Buffer* buffer = nullptr;
		UINT size = 0;
		UINT slot = 0;
	};

	struct PixelShader
	{
		ID3D11PixelShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::vector<std::pair<std::string, ConstBuffer>> constBuffers;
	};

	struct VertexShader
	{
		ID3D11VertexShader* shader = nullptr;
		ID3D11ShaderReflection* reflection = nullptr;
		std::vector<std::pair<std::string, ConstBuffer>> constBuffers;
	};

	class DirectXShaders : public Shaders
	{
	public:
		DirectXShaders(const std::string& filepath);
		DirectXShaders(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);
		~DirectXShaders();

		void Init(const std::string& vertexshaderSrc, const std::string& pixelshaderSrc);

		virtual void SetBuffer(const std::string& bufferName, const Buffer& data) override;

		void UploudBuffer(const std::string& bufferName, const Buffer& data);

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		void Bind() override;
		void UnBind() override;

		virtual const std::string& GetName() const override { return m_Name; }
	private:
		Ref<DirectXRendererAPI> m_DXApi;

		PixelShader m_PixelShader;
		VertexShader m_VertexShader;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		VertexLayout m_VertexLayout;

		std::string m_Name;
	};

}