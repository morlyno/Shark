#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Buffer.h"
#include "Shark/Render/ShaderReflection.h"

#include <nvrhi/nvrhi.h>

namespace Shark {

	struct ShaderInfo
	{
		uint64_t ShaderID = 0;
		std::filesystem::path SourcePath;
	};

	struct StageInfo
	{
		nvrhi::ShaderType Stage;
		uint64_t HashCode;
	};

	struct CompilerOptions
	{
		bool Force = false;
		bool Optimize = true;
		bool GenerateDebugInfo = false;
	};

	struct ShaderTypeMapping
	{
		const wchar_t* DXC;
		const char* D3D11;
		const char* Extension;
	};

	static std::map<nvrhi::ShaderType, ShaderTypeMapping> s_ShaderTypeMappings = {
		{ nvrhi::ShaderType::Vertex,  { .DXC = L"vs_6_0", .D3D11 = "vs_5_0", .Extension = ".vert" } },
		{ nvrhi::ShaderType::Pixel,   { .DXC = L"ps_6_0", .D3D11 = "ps_5_0", .Extension = ".pixel" } },
		{ nvrhi::ShaderType::Compute, { .DXC = L"cs_6_0", .D3D11 = "cs_5_0", .Extension = ".comp" } }
	};

	struct CompilerResult
	{
		std::map<nvrhi::ShaderType, std::vector<uint32_t>> SpirvBinary;
		std::map<nvrhi::GraphicsAPI, std::map<nvrhi::ShaderType, Buffer>> PlatformBinary;

		ShaderReflection Reflection;
		std::vector<std::string> RequestedBindingSets;
		LayoutShareMode LayoutMode = LayoutShareMode::Default;
	};

}
