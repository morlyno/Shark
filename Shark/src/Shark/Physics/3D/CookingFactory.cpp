#include "skpch.h"
#include "CookingFactory.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Mesh.h"

#include "Shark/Physics/3D/PhysicsSystem.h"
#include "Shark/Physics/3D/BinaryStream.h"
#include "Shark/Physics/3D/Utilities.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>

namespace Shark {

	void CookingFactory::CookMesh(AssetHandle meshHandle, bool forceCook)
	{
		if (!AssetManager::IsValidAssetHandle(meshHandle))
			return;

		auto& cache = PhysicsSystem::GetColliderCache();
		if (!forceCook && cache.HasCollider(meshHandle))
			return;

		Ref<Mesh> meshAsset = AssetManager::GetAsset<Mesh>(meshHandle);
		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(meshAsset->GetMeshSource());

		auto& colliderData = cache.GetColliderData(meshHandle);
		colliderData.MeshHandle = meshHandle;
		colliderData.Submeshes.clear();
		CookConvexMesh(meshSource, meshAsset->GetSubmeshes(), colliderData);
	}

	void CookingFactory::CookConvexMesh(Ref<MeshSource> meshSource, std::span<uint32_t> submeshIndices, MeshCollider& outCollider)
	{
		const auto& vertices = meshSource->GetVertices();
		const auto& indices = meshSource->GetIndices();

		for (uint32_t submeshIndex : submeshIndices)
		{
			if (!meshSource->HasSubmesh(submeshIndex))
				continue;

			const Submesh& submesh = meshSource->GetSubmesh(submeshIndex);

			JPH::Array<JPH::Vec3> positions;

			for (uint32_t i = submesh.BaseIndex / 3; i < (submesh.BaseIndex + submesh.IndexCount) / 3; i++)
			{
				const Index& index = indices[i];
				const Vertex& v0 = vertices[index.Vertex1];
				positions.push_back(JoltUtils::ToJPH(v0.Position));

				const Vertex& v1 = vertices[index.Vertex2];
				positions.push_back(JoltUtils::ToJPH(v1.Position));

				const Vertex& v2 = vertices[index.Vertex3];
				positions.push_back(JoltUtils::ToJPH(v2.Position));
			}

			auto settings = JPH::ConvexHullShapeSettings(positions);
			auto result = settings.Create();

			if (!result.IsValid())
			{
				const JPH::String& errorMessage = result.GetError();
				SK_CORE_ERROR_TAG("Physics", "Failed to create Shape: {}", errorMessage);
				continue;
			}

			JPH::Ref<JPH::Shape> shape = result.Get();

			JoltBinaryStreamWriter stream;
			shape->SaveBinaryState(stream);

			SubmeshCollider& submeshCollider = outCollider.Submeshes.emplace_back();
			submeshCollider.ShapeState = stream.GetFinalBuffer();
			submeshCollider.SubmeshIndex = submeshIndex;
		}

	}

	bool MeshColliderCache::HasCollider(AssetHandle meshHandle)
	{
		return m_Colliders.contains(meshHandle);
	}

	MeshCollider& MeshColliderCache::GetColliderData(AssetHandle meshHandle)
	{
		return m_Colliders[meshHandle];
	}

}
