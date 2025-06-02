#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Asset/Asset.h"
#include "Shark/Render/MeshSource.h"

namespace Shark {


	struct SubmeshCollider
	{
		uint32_t SubmeshIndex = 0;
		ScopedBuffer ShapeState;
	};

	struct MeshCollider
	{
		AssetHandle MeshHandle;
		std::vector<SubmeshCollider> Submeshes;
	};

	class MeshColliderCache
	{
	public:
		bool HasCollider(AssetHandle meshHandle);
		MeshCollider& GetColliderData(AssetHandle meshHandle);
	private:
		std::unordered_map<AssetHandle, MeshCollider> m_Colliders;
	};

	class CookingFactory
	{
	public:
		void CookMesh(AssetHandle meshHandle, bool forceCook = false);

	private:
		void CookConvexMesh(Ref<MeshSource> meshSource, std::span<uint32_t> submeshIndices, MeshCollider& outCollider);
	};
	
}
