#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderReflection.h"
#include "Shark/Render/ShaderCompiler/ShaderCache.h"
#include <nvrhi/nvrhi.h>

#if SK_WITH_DX11
	#include <d3d11shader.h>
#endif

namespace Shark {

	class ShaderCompiler;

#if SK_WITH_DX11

	class D3D11ShaderCompilerInterface
	{
	public:
		D3D11ShaderCompilerInterface(ShaderCompiler& compiler);
		~D3D11ShaderCompilerInterface();

		void ClearState();

		std::string CompileStage(nvrhi::ShaderType stage);
		void ReflectStage(nvrhi::ShaderType stage);

		bool LoadOrCompileStage(nvrhi::ShaderType stage, ShaderCache& cache);

		Buffer& GetBinary(nvrhi::ShaderType stage) { return m_D3D11Binary[stage]; }
		const Buffer& GetBinary(nvrhi::ShaderType stage) const { return m_D3D11Binary.at(stage); }
		const std::string& GetSource(nvrhi::ShaderType stage) const { return m_HLSLSource.at(stage); }
	private:
		void CrossCompileStage(nvrhi::ShaderType stage);
		void ReflectResource(ShaderReflection::Resource& resource);
		std::string GetReflectionName(ShaderReflection::ResourceType type, const std::string& name, uint32_t arraySize);

	private:
		ShaderCompiler& m_Compiler;
		ID3D11ShaderReflection* m_CurrentReflector = nullptr;

		std::map<nvrhi::ShaderType, std::string> m_HLSLSource;
		std::map<nvrhi::ShaderType, Buffer> m_D3D11Binary;
	};

#else

	// When DX11 is disabled this compiler produces nothing but will never fail.
	class D3D11ShaderCompilerInterface
	{
	public:
		D3D11ShaderCompilerInterface(ShaderCompiler& compiler) {}
		~D3D11ShaderCompilerInterface() { m_TempBuffer.Release(); }

		void ClearState() { m_TempBuffer.Release(); }

		std::string CompileStage(nvrhi::ShaderType stage) { (void)stage; return std::string{}; }
		void ReflectStage(nvrhi::ShaderType stage) { (void)stage; }

		bool LoadOrCompileStage(nvrhi::ShaderType stage, ShaderCache& cache) { (void)stage; (void)cache; return true }

		Buffer& GetBinary(nvrhi::ShaderType stage) const { return m_TempBuffer; }
		const std::string& GetSource(nvrhi::ShaderType stage) const { return std::string{}; }

	private:
		Buffer m_TempBuffer;
	};

#endif

}
