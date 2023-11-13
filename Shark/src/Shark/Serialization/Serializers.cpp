#include "skpch.h"
#include "Serializers.h"

#include "Shark/Render/Font.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	bool FontSerializer::Serialize(Ref<Asset> asset, const AssetMetaData& metadata)
	{
		return true;
	}

	bool FontSerializer::TryLoadAsset(Ref<Asset>& asset, const AssetMetaData& metadata)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG(Tag::Serialization, "Deserializing Font from {}", metadata.FilePath);
		Timer timer;

		auto assetManager = Project::GetActive()->GetEditorAssetManager();
		if (!assetManager->HasExistingFilePath(metadata))
		{
			SK_CORE_ERROR_TAG(Tag::Serialization, "Path not found! {0}", metadata.FilePath);
			return false;
		}

		std::filesystem::path fontPath = assetManager->GetFilesystemPath(metadata);
		Ref<Font> font = Ref<Font>::Create(fontPath);
		asset = font;
		asset->Handle = metadata.Handle;

		SK_CORE_INFO_TAG("Serialization", "Deserializing Font took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

	bool FontSerializer::Deserialize(Ref<Asset> asset, const std::filesystem::path& assetPath)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_INFO_TAG(Tag::Serialization, "Deserializing Font from {}", assetPath);
		Timer timer;

		if (!FileSystem::Exists(assetPath))
		{
			SK_CORE_ERROR_TAG(Tag::Serialization, "Path not found! {0}", assetPath);
			return false;
		}

		Ref<Font> font = asset.As<Font>();
		font->Load(FileSystem::GetFilesystemPath(assetPath));

		SK_CORE_INFO_TAG("Serialization", "Deserializing Font took {}ms", timer.ElapsedMilliSeconds());
		return true;
	}

}
