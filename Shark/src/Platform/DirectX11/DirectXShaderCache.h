#pragma once

#include "Platform/DirectX11/ShaderUtils.h"

namespace Shark {

	class DirectXShaderCompiler;

	class DirectXShaderCache
	{
	public:
		static ShaderUtils::ShaderStage::Flags HasChanged(Ref<DirectXShaderCompiler> compiler);
		static void OnShaderCompiled(Ref<DirectXShaderCompiler> compiler);

	private:
		DirectXShaderCache();
		~DirectXShaderCache();

		ShaderUtils::ShaderStage::Flags GetChangedStages(Ref<DirectXShaderCompiler> compiler) const;
		void OnShaderCompiledInternal(Ref<DirectXShaderCompiler> compiler);
	private:
		void WriteShaderSourceHashCodesToDisc();
		void ReadShaderSourceHashCodesFromDisc();

	private:
		inline static DirectXShaderCache* s_Instance = nullptr;

		std::map<std::filesystem::path, std::map<ShaderUtils::ShaderStage::Type, uint64_t>> m_CachedShaderSourceHashCodes;
	};


}
