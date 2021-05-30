#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shaders.h"

#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	enum class Shader
	{
		None = 0,
		Vertex, Pixel
	};

	class DirectXShaders : public Shaders
	{
	public:
		DirectXShaders(const std::string& filepath);
		virtual ~DirectXShaders();
		void Release();

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		virtual const std::string& GetFilePath() const override { return m_FilePath; }
		virtual const std::string& GetName() const override { return m_FileName; }

		virtual bool ReCompile() override;

		virtual void Bind() override;
		virtual void UnBind() override;

	private:
		std::string ReadFile(const std::string& filepath);
		std::unordered_map<Shader, std::string> PreProzess(const std::string& file);
		bool TryReCompile(std::unordered_map<Shader, std::string>& shaderSources);
		void CompileOrGetCached(std::unordered_map<Shader, std::string>& shaderSources);
		void Reflect();
		void CreateInputlayout(const std::vector<byte>& vtx_src);
		void CreateShaders();

	private:
		ID3D11PixelShader* m_PixelShader;
		ID3D11VertexShader* m_VertexShader;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		std::unordered_map<Shader, std::vector<byte>> m_ShaderBinarys;
		std::string m_FilePath;
		std::string m_FileName;

		VertexLayout m_VertexLayout;

	};

}