#include "skpch.h"
#include "Serializers.h"

#include "Shark/Asset/Assets.h"

#include "Shark/Render/Font.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool ScriptFileSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		SK_CORE_VERIFY(asset);
		return true;
	}

	bool ScriptFileSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		asset = Ref<ScriptFile>::Create();
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

	bool FontSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool FontSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();

		auto filesystemPath = context->GetFilesystemPath(metadata);
		if (!FileSystem::Exists(filesystemPath))
		{
			context->OnFileNotFound(metadata);
			return false;
		}

		Ref<Font> font = Ref<Font>::Create(filesystemPath);
		asset = font;
		asset->Handle = metadata.Handle;
		context->SetStatus(AssetLoadStatus::Ready);
		return true;
	}

}
