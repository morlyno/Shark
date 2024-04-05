#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Image.h"

namespace Shark {

	class ThumbnailCache : public RefCount
	{
	public:
		ThumbnailCache();
		~ThumbnailCache();

		void Clear();
		bool HasThumbnail(AssetHandle assetHandle);
		bool IsThumbnailCurrent(AssetHandle assetHandle);
		Ref<Image2D> GetThumbnail(AssetHandle assetHandle);
		void SetThumbnail(AssetHandle assetHandle, Ref<Image2D> thumbnail);

	private:
		struct ThumbnailImage
		{
			Ref<Image2D> Thumbnail;
			uint64_t LastWriteTime = 0;
		};

		std::unordered_map<AssetHandle, ThumbnailImage> m_Thumbnails;
	};

}
