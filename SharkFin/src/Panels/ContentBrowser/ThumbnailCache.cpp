#include "skfpch.h"
#include "ThumbnailCache.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/File/Serialization/FileStream.h"

namespace Shark {

	namespace utils {

		static std::filesystem::path GetCacheDirectory()
		{
			return "Cache/Thumbnail";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
			const auto cacheDirectory = GetCacheDirectory();
			if (!FileSystem::Exists(cacheDirectory))
				FileSystem::CreateDirectories(cacheDirectory);
		}

		static std::filesystem::path GetThumbnailCacheFilepath(AssetHandle handle)
		{
			return GetCacheDirectory() / fmt::format("{}.skthumb", handle);
		}

	}

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
		if (m_Thumbnails.contains(assetHandle))
			return true;

		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(assetHandle);
		return FileSystem::Exists(cacheFile);
	}

	bool ThumbnailCache::IsThumbnailCurrent(AssetHandle handle)
	{
		if (!HasThumbnail(handle))
			return false;

		uint64_t thumbnailTimestamp = 0;
		if (m_Thumbnails.contains(handle))
			thumbnailTimestamp = m_Thumbnails.at(handle).Timestamp;
		else
			thumbnailTimestamp = ReadTimestampFromCache(handle);

		uint64_t lastWriteTime = Project::GetActiveEditorAssetManager()->GetMetadata(handle).LastWriteTime;
		if (lastWriteTime == 0)
			lastWriteTime = FileSystem::GetLastWriteTime(Project::GetActiveEditorAssetManager()->GetFilesystemPath(handle));

		return thumbnailTimestamp == lastWriteTime;
	}

	Ref<Image2D> ThumbnailCache::GetThumbnail(AssetHandle handle)
	{
		if (!HasThumbnail(handle))
			return nullptr;

		if (!m_Thumbnails.contains(handle))
			LoadThumbnailFromDisc(handle);

		return m_Thumbnails.at(handle).Thumbnail;
	}

	void ThumbnailCache::SetThumbnail(AssetHandle assetHandle, Ref<Image2D> thumbnail)
	{
		uint64_t lastWriteTime = Project::GetActiveEditorAssetManager()->GetMetadata(assetHandle).LastWriteTime;
		m_Thumbnails[assetHandle] = { thumbnail, lastWriteTime };

		Renderer::Submit([=]()
		{
			WriteThumbnailToDisc(assetHandle, thumbnail, lastWriteTime);
		});
	}

	void ThumbnailCache::WriteThumbnailToDisc(AssetHandle handle, Ref<Image2D> image, uint64_t lastWriteTime)
	{
		utils::CreateCacheDirectoryIfNeeded();
		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(handle);

		Buffer imageData;
		image->RT_CopyToHostBuffer(imageData);

		ThumbnailFileHeader header;
		header.Timestamp = lastWriteTime;
		header.Width = image->GetWidth();
		header.Height = image->GetHeight();

		FileStreamWriter stream(cacheFile);
		stream.WriteRaw(header);
		stream.WriteBuffer(imageData);
	}

	bool ThumbnailCache::LoadThumbnailFromDisc(AssetHandle handle)
	{
		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(handle);
		if (!FileSystem::Exists(cacheFile))
			return false;

		FileStreamReader stream(cacheFile);

		ThumbnailFileHeader header;
		stream.ReadRaw(header);

		Buffer imageData;
		stream.ReadBuffer(imageData);

		ImageSpecification specification;
		specification.Format = ImageFormat::RGBA8;
		specification.Width = header.Width;
		specification.Height = header.Height;
		Ref<Image2D> image = Image2D::Create(specification);
		image->UploadImageData(imageData);
		m_Thumbnails[handle] = { image, header.Timestamp };
	}

	uint64_t ThumbnailCache::ReadTimestampFromCache(AssetHandle handle)
	{
		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(handle);
		if (!FileSystem::Exists(cacheFile))
			return false;

		FileStreamReader stream(cacheFile);

		ThumbnailFileHeader header;
		stream.ReadRaw(header);

		return header.Timestamp;
	}

}
