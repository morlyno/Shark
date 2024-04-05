#include "skpch.h"
#include "Mesh.h"

#include "Shark/Asset/AssetManager.h"

namespace Shark {

	Mesh::Mesh(AssetHandle meshSource)
		: m_MeshSource(meshSource)
	{
	}

	Mesh::Mesh(AssetHandle meshSource, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource), m_Submeshes(submeshes)
	{
	}

}
