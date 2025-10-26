#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderReflection.h"
#include "Shark/Render/ShaderCompiler/ShaderCache.h"
#include <nvrhi/nvrhi.h>

namespace Shark {

	class ShaderCompiler;

#if SK_WITH_DX11

	class D3D11ShaderCompilerInterface
	{
	public:
		D3D11ShaderCompilerInterface();
		~D3D11ShaderCompilerInterface();

		void ClearState();
		void SetContext(const ShaderInfo& info, const CompilerOptions& options) { m_Info = info; m_Options = options; }

		bool CompileOrLoad(const ShaderCompiler& compiler, ShaderCache& cache);

	public:
		Buffer& GetBinary(nvrhi::ShaderType stage) { return m_D3D11Binary[stage]; }
		const Buffer& GetBinary(nvrhi::ShaderType stage) const { return m_D3D11Binary.at(stage); }
		const std::string& GetSource(nvrhi::ShaderType stage) const { return m_HLSLSource.at(stage); }

	private:
		std::string CompileStage(nvrhi::ShaderType stage);
		void CrossCompileStage(nvrhi::ShaderType stage, std::span<const uint32_t> spirvBinary, const ShaderCompiler& compiler);

		bool LoadOrCompileStage(nvrhi::ShaderType stage, ShaderCache& cache);

	private:
		ShaderInfo m_Info;
		CompilerOptions m_Options;

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
