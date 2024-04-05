#include "skfpch.h"
#include "ThumbnailCache.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/File/FileSystem.h"

namespace Shark {

	ThumbnailCache::ThumbnailCache()
	{

	}

	ThumbnailCache::~ThumbnailCache()
	{

	}

	void ThumbnailCache::Clear()
	{
		m_Thumbnails.clear();
	}

	bool ThumbnailCache::HasThumbnail(AssetHandle assetHandle)
	{
		return m_Thumbnails.contains(assetHandle);
	}

	bool ThumbnailCache::IsThumbnailCurrent(AssetHandle assetHandle)
	{
		if (!HasThumbnail(assetHandle) || !AssetManager::IsValidAssetHandle(assetHandle))
			return false;

		uint64_t lastWriteTime = FileSystem::GetLastWriteTime(Project::GetActiveEditorAssetManager()->GetFilesystemPath(assetHandle));
		return m_Thumbnails.at(assetHandle).LastWriteTime == lastWriteTime;
	}

	Ref<Image2D> ThumbnailCache::GetThumbnail(AssetHandle assetHandle)
	{
		if (!HasThumbnail(assetHandle))
			return nullptr;

		return m_Thumbnails.at(assetHandle).Thumbnail;
	}

	void ThumbnailCache::SetThumbnail(AssetHandle assetHandle, Ref<Image2D> thumbnail)
	{
		uint64_t lastWriteTime = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle).LastWriteTime;
		m_Thumbnails[assetHandle] = { thumbnail, lastWriteTime };
	}

}
