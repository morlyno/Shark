#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Shader.h"

#include <d3d11.h>
#include <d3d11shader.h>

namespace Shark {

	enum class ShaderStage
	{
		None = 0,
		Vertex, Pixel
	};

	class DirectXShader : public Shader
	{
	public:
		DirectXShader(const std::filesystem::path& filepath);
		virtual ~DirectXShader();
		void Release();

		virtual VertexLayout& GetVertexLayout() override { return m_VertexLayout; };

		virtual const std::filesystem::path& GetFilePath() const override { return m_FilePath; }
		virtual const std::string& GetFileName() const override { return m_FileName; }

		virtual bool ReCompile() override;
		virtual void PrintReflection() override { Reflect(); }

		virtual Ref<ConstantBuffer> CreateConstantBuffer(const std::string& name);

		virtual void Bind(Ref<RenderCommandBuffer> commandBuffer) override;
		virtual void UnBind(Ref<RenderCommandBuffer> commandBuffer) override;

		void Bind(ID3D11DeviceContext* ctx);
		void UnBind(ID3D11DeviceContext* ctx);

		std::unordered_map<ShaderStage, std::vector<byte>> GetShaderBinarys() const { return m_ShaderBinarys; }
	private:
		std::string ReadFile(const std::filesystem::path& filepath);
		std::unordered_map<ShaderStage, std::string> PreProzess(const std::string& file);
		bool TryReCompile(std::unordered_map<ShaderStage, std::string>& shaderSources);
		void CompileOrGetCached(std::unordered_map<ShaderStage, std::string>& shaderSources);
		void Reflect();
		void CreateInputlayout(const std::vector<byte>& vtx_src);
		void CreateShaders();

	private:
		ID3D11PixelShader* m_PixelShader = nullptr;
		ID3D11VertexShader* m_VertexShader = nullptr;
		
		ID3D11InputLayout* m_InputLayout = nullptr;

		std::unordered_map<ShaderStage, std::vector<byte>> m_ShaderBinarys;
		std::filesystem::path m_FilePath;
		std::string m_FileName;
		std::filesystem::path m_CacheFilePath;

		VertexLayout m_VertexLayout;

		friend class DirectXRenderer;

	};

}