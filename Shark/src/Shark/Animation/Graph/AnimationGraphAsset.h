#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"

namespace Shark {
	class StreamWriter;
	class StreamReader;
	class AssetLoadContext;
}

namespace Shark {

	class AnimationGraphAsset : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::AnimationGraph; }
		virtual AssetType GetAssetType() const final { return GetStaticType(); }

		AssetHandle GetSkeletonMesh() const { return m_SkeletonMesh; }
		void SetSkeletonMesh(AssetHandle skeletonMesh) { m_SkeletonMesh = skeletonMesh; }

	protected:
		AssetHandle m_SkeletonMesh;

	};

}
