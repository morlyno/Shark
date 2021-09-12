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
		DirectXShaders(const std::filesystem::path& filepath);
		virtual ~DirectXShaders();
		void Release();

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }
		virtual const std::string& GetFileName() const override { return m_FileName; }

		virtual bool ReCompile() override;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(const std::string& name);

		virtual void Bind() override;
		virtual void UnBind() override;

	private:
		std::string ReadFile(const std::filesystem::path& filepath);
		std::unordered_map<Shader, std::string> PreProzess(const std::string& file);
		bool TryReCompile(std::unordered_map<Shader, std::string>& shaderSources);
		void CompileOrGetCached(std::unordered_map<Shader, std::string>& shaderSources);
		void Reflect();
		void CreateInputlayout(const std::vector<byte>& vtx_src);
		void CreateShaders();

	private:
		ID3D11PixelShader* m_PixelShader = nullptr;
		ID3D11VertexShader* m_VertexShader = nullptr;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		std::unordered_map<Shader, std::vector<byte>> m_ShaderBinarys;
		std::filesystem::path m_FilePath;
		std::string m_FileName;
		std::filesystem::path m_CacheFilePath;

		VertexLayout m_VertexLayout;

	};

}