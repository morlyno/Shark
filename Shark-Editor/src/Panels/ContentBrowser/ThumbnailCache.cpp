#include "ThumbnailCache.h"

#include "Shark/Core/Application.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"
#include "Shark/File/FileSystem.h"
#include "Shark/File/Serialization/FileStream.h"
#include "Shark/Debug/Profiler.h"

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
		SK_CORE_WARN_TAG("ThumbnailCache", "In memory cache cleared");
		m_Thumbnails.clear();
	}

	void ThumbnailCache::ClearDiscCache()
	{
		SK_CORE_WARN_TAG("ThumbnailCache", "Disc cache cleared");
		FileSystem::RemoveAll(utils::GetCacheDirectory());
	}

	bool ThumbnailCache::HasThumbnail(AssetHandle assetHandle)
	{
		SK_PROFILE_FUNCTION();
		if (m_Thumbnails.contains(assetHandle))
			return true;

		if (!LoadFromCacheAllowed())
			return false;

		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(assetHandle);
		return FileSystem::Exists(cacheFile);
	}

	bool ThumbnailCache::IsThumbnailCurrent(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();
		if (!HasThumbnail(handle))
			return false;

		if (!Project::GetEditorAssetManager()->HasExistingFilePath(handle))
			return true;

		uint64_t thumbnailTimestamp = 0;
		if (m_Thumbnails.contains(handle))
			thumbnailTimestamp = m_Thumbnails.at(handle).Timestamp;
		else
			thumbnailTimestamp = ReadTimestampFromCache(handle);

		const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(handle);
		uint64_t lastWriteTime = metadata.LastWriteTime;
		if (lastWriteTime == 0/* && Project::GetActiveEditorAssetManager()->HasExistingFilePath(metadata)*/)
			lastWriteTime = FileSystem::GetLastWriteTime(Project::GetEditorAssetManager()->GetFilesystemPath(metadata));

		const bool current = thumbnailTimestamp == lastWriteTime;
		if (!current)
		{
			SK_CORE_TRACE_TAG("ThumbnailCache", "Out of dat thumbnail found [{}] {}", metadata.Handle, metadata.FilePath);
			return false;
		}
		return true;
	}

	Ref<Image2D> ThumbnailCache::GetThumbnail(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();
		if (!HasThumbnail(handle))
			return nullptr;

		if (!m_Thumbnails.contains(handle))
			if (!LoadThumbnailFromDisc(handle))
				return nullptr;

		return m_Thumbnails.at(handle).Thumbnail;
	}

	void ThumbnailCache::SetThumbnail(AssetHandle assetHandle, Ref<Image2D> thumbnail)
	{
		uint64_t lastWriteTime = Project::GetEditorAssetManager()->GetMetadata(assetHandle).LastWriteTime;
		m_Thumbnails[assetHandle] = { thumbnail, lastWriteTime };
		SK_CORE_TRACE_TAG("ThumbnailCache", "Thumbnail set (Handle={}, Timestamp={})", assetHandle, lastWriteTime);

		Renderer::Submit([=]()
		{
			WriteThumbnailToDisc(assetHandle, thumbnail, lastWriteTime);
		});
	}

	void ThumbnailCache::WriteThumbnailToDisc(AssetHandle handle, Ref<Image2D> image, uint64_t lastWriteTime)
	{
		SK_CORE_INFO_TAG("ThumbnailCache", "Writing Thumbnail to disc (Handle={}, Timestamp={})", handle, lastWriteTime);

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
		SK_PROFILE_FUNCTION();
		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(handle);
		if (!FileSystem::Exists(cacheFile))
			return false;

		FileStreamReader stream(cacheFile);

		ThumbnailFileHeader header;
		stream.ReadRaw(header);

		Buffer imageData;
		stream.ReadBuffer(imageData);

		ImageSpecification specification;
		specification.Format = ImageFormat::RGBA8UNorm;
		specification.Width = header.Width;
		specification.Height = header.Height;
		Ref<Image2D> image = Image2D::Create(specification);
		image->UploadImageData(imageData);
		m_Thumbnails[handle] = { image, header.Timestamp };
		imageData.Release();

		m_LoadCount++;
		SK_CORE_INFO_TAG("ThumbnailCache", "[FI={}] Loaded Thumbnail from disc (Handle={}, Timestamp={}", Application::Get().GetFrameCount(), handle, header.Timestamp);
		return true;
	}

	uint64_t ThumbnailCache::ReadTimestampFromCache(AssetHandle handle)
	{
		SK_PROFILE_FUNCTION();
		std::filesystem::path cacheFile = utils::GetThumbnailCacheFilepath(handle);
		if (!FileSystem::Exists(cacheFile))
			return 0;

		FileStreamReader stream(cacheFile);

		ThumbnailFileHeader header;
		stream.ReadRaw(header);

		return header.Timestamp;
	}

	bool ThumbnailCache::LoadFromCacheAllowed()
	{
		const uint64_t appFrameIndex = Application::Get().GetFrameCount();
		if (appFrameIndex != m_CurrentFrame)
		{
			m_LoadCount = 0;
			m_CurrentFrame = appFrameIndex;
		}

		return m_LoadCount < m_MaxLoadsPerFrame;
	}

}
