#include "skpch.h"
#include "DirectXShaderCache.h"

#include "Shark/File/FileSystem.h"
#include "Platform/DirectX11/DirectXShaderCompiler.h"

#include <yaml-cpp/yaml.h>
#include "Shark/Utils/YAMLUtils.h"

namespace YAML {

	template<>
	struct convert<Shark::ShaderUtils::ShaderStage::Type>
	{
		static bool decode(const Node& node, Shark::ShaderUtils::ShaderStage::Type& shaderStage)
		{
			if (!node.IsScalar())
				return false;

			shaderStage = Shark::StringToShaderStage(node.Scalar());
			return true;
		}
	};

	Emitter& operator<<(Emitter& out, const Shark::ShaderUtils::ShaderStage::Type& shaderStage)
	{
		return out << Shark::ToString(shaderStage);
	}

}

namespace Shark {

	namespace utils {

		static std::filesystem::path GetShaderCacheRegistryFilePath()
		{
			return FileSystem::GetResourcePath(L"Cache/Shaders/ShaderCacheRegistry.yaml");
		}

	}

	Shark::ShaderUtils::ShaderStage::Flags DirectXShaderCache::HasChanged(Ref<DirectXShaderCompiler> compiler)
	{
		if (!s_Instance)
		{
			s_Instance = sknew DirectXShaderCache();
			s_Instance->ReadShaderSourceHashCodesFromDisc();
		}

		return s_Instance->GetChangedStages(compiler);
	}

	void DirectXShaderCache::OnShaderCompiled(Ref<DirectXShaderCompiler> compiler)
	{
		if (!s_Instance)
		{
			s_Instance = sknew DirectXShaderCache();
			s_Instance->ReadShaderSourceHashCodesFromDisc();
		}

		s_Instance->OnShaderCompiledInternal(compiler);
	}

	DirectXShaderCache::DirectXShaderCache()
	{
	}

	DirectXShaderCache::~DirectXShaderCache()
	{
	}

	ShaderUtils::ShaderStage::Flags DirectXShaderCache::GetChangedStages(Ref<DirectXShaderCompiler> compiler) const
	{
		if (!m_CachedShaderSourceHashCodes.contains(compiler->GetShaderSourcePath()))
			return ShaderUtils::ShaderStage::All;

		ShaderUtils::ShaderStage::Flags changedStages = ShaderUtils::ShaderStage::None;

		auto& hashCodes = m_CachedShaderSourceHashCodes.at(compiler->GetShaderSourcePath());
		for (const auto& [stage, metadata] : compiler->m_ShaderStageMetadata)
		{
			if (!std::filesystem::exists(metadata.CacheFile))
			{
				changedStages |= stage;
				continue;
			}

			if (!hashCodes.contains(stage))
			{
				changedStages |= stage;
				continue;
			}

			if (hashCodes.at(stage) != metadata.HashCode)
			{
				changedStages |= stage;
			}
		}

		return changedStages;
	}

	void DirectXShaderCache::OnShaderCompiledInternal(Ref<DirectXShaderCompiler> compiler)
	{
		auto& hashCodes = m_CachedShaderSourceHashCodes[compiler->GetShaderSourcePath()];

		for (const auto& [stage, metadata] : compiler->m_ShaderStageMetadata)
		{
			// Only update HashCodes when the Cache changed
			if (!(compiler->m_StagesWrittenToCache & stage))
				continue;

			hashCodes[stage] = metadata.HashCode;
		}

		WriteShaderSourceHashCodesToDisc();
	}

	void DirectXShaderCache::WriteShaderSourceHashCodesToDisc()
	{
		YAML::Emitter out;

		out << YAML::BeginMap;
		out << YAML::Key << "ShaderCache" << YAML::Value;
		out << YAML::BeginSeq;

		for (const auto& [sourceFile, hashCodes] : m_CachedShaderSourceHashCodes)
		{
			if (!std::filesystem::exists(sourceFile))
				continue;

			out << YAML::BeginMap;
			out << YAML::Key << "SourceFile" << YAML::Value << sourceFile;
			out << YAML::Key << "HashCodes" << YAML::Value << hashCodes;
			out << YAML::EndMap;
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		if (!FileSystem::WriteString(utils::GetShaderCacheRegistryFilePath(), out.c_str()))
			SK_CORE_ERROR_TAG("Renderer", "Failed to Write Shader Cache to Disc!");
	}

	void DirectXShaderCache::ReadShaderSourceHashCodesFromDisc()
	{
		std::string fileData = FileSystem::ReadString(utils::GetShaderCacheRegistryFilePath());
		if (fileData.empty())
		{
			SK_CORE_WARN_TAG("Renderer", "Failed to Read Shader Cache from Disc");
			return;
		}

		YAML::Node in = YAML::Load(fileData);
		SK_CORE_ASSERT(in);

		auto shaderCacheNode = in["ShaderCache"];
		if (!shaderCacheNode)
			return;

		for (auto node : shaderCacheNode)
		{
			auto sourceFile = node["SourceFile"].as<std::filesystem::path>();
			auto hashCodes = node["HashCodes"].as<std::map<ShaderUtils::ShaderStage::Type, uint64_t>>();

			if (!std::filesystem::exists(sourceFile))
				continue;

			m_CachedShaderSourceHashCodes[sourceFile] = std::move(hashCodes);
		}
	}

}
