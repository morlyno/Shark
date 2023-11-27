#include "skpch.h"
#include "Mesh.h"

namespace Shark {

	Mesh::Mesh(Ref<MeshSource> meshSource)
		: m_MeshSource(meshSource), m_MaterialTable(Ref<MaterialTable>::Create())
	{
	}

	Mesh::Mesh(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes)
		: m_MeshSource(meshSource), m_MaterialTable(materials), m_Submeshes(submeshes)
	{
	}

	Ref<Mesh> Mesh::Create(Ref<MeshSource> meshSource)
	{
		return Ref<Mesh>::Create(meshSource);
	}

	Ref<Mesh> Mesh::Create(Ref<MeshSource> meshSource, Ref<MaterialTable> materials, const std::vector<uint32_t>& submeshes)
	{
		return Ref<Mesh>::Create(meshSource, materials, submeshes);
	}

}
