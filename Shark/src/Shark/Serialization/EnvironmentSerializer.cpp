#include "skpch.h"
#include "EnvironmentSerializer.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Environment.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool EnvironmentSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool EnvironmentSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Loading EnvironmentMap from {}", metadata.FilePath);
		ScopedTimer timer("Loading EnvironmentMap");

		const auto filesystemPath = Project::GetActiveEditorAssetManager()->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			SK_CORE_ERROR_TAG("Serialization", "Path not found! {}", metadata.FilePath);
			return false;
		}

		auto [radianceMap, irradianceMap] = Renderer::CreateEnvironmentMap(filesystemPath);
		Ref<Environment> environment = Ref<Environment>::Create(radianceMap, irradianceMap);
		asset = environment;
		asset->Handle = metadata.Handle;
		return true;
	}

}
