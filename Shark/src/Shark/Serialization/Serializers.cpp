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
		SK_CORE_INFO_TAG("Serialization", "Serializing ScriptFile to {}", metadata.FilePath);
		Timer timer;

		SK_CORE_INFO_TAG("Serialization", "Serializing ScriptFile took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool ScriptFileSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Deserializing ScriptFile from {}", metadata.FilePath);
		Timer timer;

		asset = Ref<ScriptFile>::Create();
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing ScriptFile took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool FontSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool FontSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata, AssetLoadContext* context)
	{
		SK_PROFILE_FUNCTION();
		SK_CORE_INFO_TAG("Serialization", "Deserializing Font from {}", metadata.FilePath);
		Timer timer;

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

		SK_CORE_INFO_TAG("Serialization", "Deserializing Font took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

}
