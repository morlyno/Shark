#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/ShaderReflection.h"

#include <nvrhi/nvrhi.h>
#include <shaderc/shaderc.h>

namespace Shark {

	struct ShaderInfo
	{
		uint64_t ShaderID = 0;
		std::filesystem::path SourcePath;
	};

	struct ShaderSourceInfo
	{
		nvrhi::ShaderType Stage;
		uint64_t HashCode;
		uint64_t ShaderID;

		std::string Source;
	};

	struct CompilerOptions
	{
		bool Force = false;
		bool Optimize = true;
		bool GenerateDebugInfo = false;
	};

	struct ShaderTypeMapping
	{
		shaderc_shader_kind ShaderC;
		const wchar_t* DXC;
		const char* D3D11;
		const char* Extension;
	};

	static std::map<nvrhi::ShaderType, ShaderTypeMapping> s_ShaderTypeMappings = {
		{ nvrhi::ShaderType::Vertex,  {.ShaderC = shaderc_vertex_shader,   .DXC = L"vs_6_0", .D3D11 = "vs_5_0", .Extension = ".vert" } },
		{ nvrhi::ShaderType::Pixel,   {.ShaderC = shaderc_fragment_shader, .DXC = L"ps_6_0", .D3D11 = "ps_5_0", .Extension = ".pixel" } },
		{ nvrhi::ShaderType::Compute, {.ShaderC = shaderc_compute_shader,  .DXC = L"cs_6_0", .D3D11 = "cs_5_0", .Extension = ".comp" } }
	};

}
