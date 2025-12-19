#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Image.h"
#include "Shark/Render/RenderCommandBuffer.h"

namespace Shark {

	class ThumbnailCache : public RefCount
	{
	public:
		ThumbnailCache();
		~ThumbnailCache();

		void Clear();
		void ClearDiscCache();

		void ClearFor(std::span<const AssetHandle> handles);
		void ClearDiscFor(std::span<const AssetHandle> handles);

		bool HasThumbnail(AssetHandle assetHandle);
		bool IsThumbnailCurrent(AssetHandle assetHandle);
		Ref<Image2D> GetThumbnail(AssetHandle assetHandle);
		void SetThumbnail(AssetHandle assetHandle, Ref<Image2D> thumbnail);

	private:
		static void WriteThumbnailToDisc(AssetHandle handle, Ref<StagingImage2D> thumbnail, uint64_t lastWriteTime);
		bool LoadThumbnailFromDisc(AssetHandle handle);
		uint64_t ReadTimestampFromCache(AssetHandle handle);
		bool LoadFromCacheAllowed();

	private:
		struct ThumbnailImage
		{
			Ref<Image2D> Thumbnail;
			uint64_t Timestamp = 0;
		};

		std::unordered_map<AssetHandle, ThumbnailImage> m_Thumbnails;
		Ref<StagingImage2D> m_ReadableImage;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		struct ThumbnailFileHeader
		{
			const char Header[4] = { 'S', 'K', 'T', 'N' };
			uint16_t Version = 1;
			uint16_t Flags = 0;
			uint64_t Timestamp = 0;

			uint32_t Width = 0;
			uint32_t Height = 0;
		};

		uint32_t m_LoadCount = 0;
		uint32_t m_MaxLoadsPerFrame = 1;
		uint64_t m_CurrentFrame = 0;
	};

}
