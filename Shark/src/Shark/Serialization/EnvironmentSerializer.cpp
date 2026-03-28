#include "skpch.h"
#include "EnvironmentSerializer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Environment.h"

#include "Shark/File/FileSystem.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool EnvironmentSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool EnvironmentSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading EnvironmentMap from {}", metadata.FilePath);

		ScopedTimer timer("Loading EnvironmentMap");

		const auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		Ref<Environment> environment;

		auto [radianceMap, irradianceMap] = Renderer::MT::CreateEnvironmentMap(filesystemPath);
		environment = Ref<Environment>::Create(radianceMap, irradianceMap);

		asset = environment;
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

}
