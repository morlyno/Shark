#include "skpch.h"
#include "MeshFactory.h"

#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"

#include <glm/gtx/compatibility.hpp>

namespace Shark {

	struct MeshFactoryCache
	{
		AssetHandle CubeHandle;
		AssetHandle SphereHandle;
	};
	MeshFactoryCache s_MeshFactoryCache;

	Ref<MeshSource> MeshFactory::CreateCube()
	{
		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" },
			{ VertexDataType::Float3, "Normal" },
			{ VertexDataType::Float2, "UV" }
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 UV;
		};

		Vertex vertices[] = {
			{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{  0.0f,  0.0f, -1.0f }, glm::vec2{ 0.0f, 0.0f } }, // near
			{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{  0.0f,  0.0f, -1.0f }, glm::vec2{ 1.0f, 0.0f } }, // near
			{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{  0.0f,  0.0f, -1.0f }, glm::vec2{ 0.0f, 1.0f } }, // near
			{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{  0.0f,  0.0f, -1.0f }, glm::vec2{ 1.0f, 1.0f } }, // near
			{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{  0.0f,  0.0f,  1.0f }, glm::vec2{ 0.0f, 0.0f } }, // far
			{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{  0.0f,  0.0f,  1.0f }, glm::vec2{ 1.0f, 0.0f } }, // far
			{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{  0.0f,  0.0f,  1.0f }, glm::vec2{ 0.0f, 1.0f } }, // far
			{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{  0.0f,  0.0f,  1.0f }, glm::vec2{ 1.0f, 1.0f } }, // far
			{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{ -1.0f,  0.0f,  0.0f }, glm::vec2{ 0.0f, 0.0f } }, // left
			{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{ -1.0f,  0.0f,  0.0f }, glm::vec2{ 1.0f, 0.0f } }, // left
			{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{ -1.0f,  0.0f,  0.0f }, glm::vec2{ 0.0f, 1.0f } }, // left
			{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{ -1.0f,  0.0f,  0.0f }, glm::vec2{ 1.0f, 1.0f } }, // left
			{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{  1.0f,  0.0f,  0.0f }, glm::vec2{ 0.0f, 0.0f } }, // right
			{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{  1.0f,  0.0f,  0.0f }, glm::vec2{ 1.0f, 0.0f } }, // right
			{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{  1.0f,  0.0f,  0.0f }, glm::vec2{ 0.0f, 1.0f } }, // right
			{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{  1.0f,  0.0f,  0.0f }, glm::vec2{ 1.0f, 1.0f } }, // right
			{ glm::vec3{ -0.5f, -0.5f, -0.5f }, glm::vec3{  0.0f, -1.0f,  0.0f }, glm::vec2{ 0.0f, 0.0f } }, // bottem
			{ glm::vec3{  0.5f, -0.5f, -0.5f }, glm::vec3{  0.0f, -1.0f,  0.0f }, glm::vec2{ 1.0f, 0.0f } }, // bottem
			{ glm::vec3{ -0.5f, -0.5f,  0.5f }, glm::vec3{  0.0f, -1.0f,  0.0f }, glm::vec2{ 0.0f, 1.0f } }, // bottem
			{ glm::vec3{  0.5f, -0.5f,  0.5f }, glm::vec3{  0.0f, -1.0f,  0.0f }, glm::vec2{ 1.0f, 1.0f } }, // bottem
			{ glm::vec3{ -0.5f,  0.5f, -0.5f }, glm::vec3{  0.0f,  1.0f,  0.0f }, glm::vec2{ 0.0f, 0.0f } }, // top
			{ glm::vec3{  0.5f,  0.5f, -0.5f }, glm::vec3{  0.0f,  1.0f,  0.0f }, glm::vec2{ 1.0f, 0.0f } }, // top
			{ glm::vec3{ -0.5f,  0.5f,  0.5f }, glm::vec3{  0.0f,  1.0f,  0.0f }, glm::vec2{ 0.0f, 1.0f } }, // top
			{ glm::vec3{  0.5f,  0.5f,  0.5f }, glm::vec3{  0.0f,  1.0f,  0.0f }, glm::vec2{ 1.0f, 1.0f } }  // top
		};

		uint32_t indices[] = {
			 0, 2, 1,  2, 3, 1,
			 4, 5, 7,  4, 7, 6,
			 8,10, 9, 10,11, 9,
			12,13,15, 12,15,14,
			16,17,18, 18,17,19,
			20,23,21, 20,22,23
		};

		auto vertexBuffer = VertexBuffer::Create(layout, Buffer::FromArray(vertices));
		auto indexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
		auto materialTable = Ref<MaterialTable>::Create();
		auto material = MaterialAsset::Create();
		material->UpdateMaterialIfDirty();
		materialTable->AddMaterial(0, material);

		return MeshSource::Create(vertexBuffer, indexBuffer, materialTable);
	}

	Ref<MeshSource> MeshFactory::CreateSphere(int latDiv, int longDiv)
	{
		SK_CORE_VERIFY(latDiv >= 3);
		SK_CORE_VERIFY(longDiv >= 3);

		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" },
			{ VertexDataType::Float3, "Normal" },
			{ VertexDataType::Float2, "UV" }
		};

		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 UV;
		};

		constexpr float radius = 1.0f;
		const glm::vec4 base = { 0.0f, 0.0f, radius, 0.0f };
		const float lattitudeAngle = glm::pi<float>() / latDiv;
		const float longitudeAngle = glm::two_pi<float>() / longDiv;

		std::vector<Vertex> vertices;

		for (int iLat = 1; iLat < latDiv; iLat++)
		{
			const glm::vec4 latBase = glm::toMat4(glm::quat(glm::vec3(lattitudeAngle * iLat, 0.0f, 0.0f))) * base;

			for (int iLong = 0; iLong < longDiv; iLong++)
			{
				auto& vertex = vertices.emplace_back();
				vertex.Position = glm::rotate(longitudeAngle * iLong, glm::vec3{ 0.0f, 0.0f, 1.0f }) * latBase;
			}
		}

		// add the cap vertices
		const auto iNorthPole = (unsigned short)vertices.size();
		vertices.emplace_back().Position = base;

		const auto iSouthPole = (unsigned short)vertices.size();
		vertices.emplace_back().Position = -base;

		for (auto& vertex : vertices)
		{
			vertex.Normal = glm::normalize(vertex.Position);
			vertex.UV.x = glm::atan2(vertex.Normal.x, vertex.Normal.z) / glm::two_pi<float>() + 0.5f;
			vertex.UV.y = vertex.Normal.y * 0.5f + 0.5f;
		}

		std::vector<uint32_t> indices;

		const auto calcIdx = [latDiv, longDiv](unsigned short iLat, unsigned short iLong)
		{ return iLat * longDiv + iLong; };
		for (unsigned short iLat = 0; iLat < latDiv - 2; iLat++)
		{
			for (unsigned short iLong = 0; iLong < longDiv - 1; iLong++)
			{
				indices.push_back(calcIdx(iLat, iLong));
				indices.push_back(calcIdx(iLat + 1, iLong));
				indices.push_back(calcIdx(iLat, iLong + 1));
				indices.push_back(calcIdx(iLat, iLong + 1));
				indices.push_back(calcIdx(iLat + 1, iLong));
				indices.push_back(calcIdx(iLat + 1, iLong + 1));
			}
			// wrap band
			indices.push_back(calcIdx(iLat, longDiv - 1));
			indices.push_back(calcIdx(iLat + 1, longDiv - 1));
			indices.push_back(calcIdx(iLat, 0));
			indices.push_back(calcIdx(iLat, 0));
			indices.push_back(calcIdx(iLat + 1, longDiv - 1));
			indices.push_back(calcIdx(iLat + 1, 0));
		}

		// cap fans
		for (unsigned short iLong = 0; iLong < longDiv - 1; iLong++)
		{
			// north
			indices.push_back(iNorthPole);
			indices.push_back(calcIdx(0, iLong));
			indices.push_back(calcIdx(0, iLong + 1));
			// south
			indices.push_back(calcIdx(latDiv - 2, iLong + 1));
			indices.push_back(calcIdx(latDiv - 2, iLong));
			indices.push_back(iSouthPole);
		}
		// wrap triangles
		// north
		indices.push_back(iNorthPole);
		indices.push_back(calcIdx(0, longDiv - 1));
		indices.push_back(calcIdx(0, 0));
		// south
		indices.push_back(calcIdx(latDiv - 2, 0));
		indices.push_back(calcIdx(latDiv - 2, longDiv - 1));
		indices.push_back(iSouthPole);

		auto vertexBuffer = VertexBuffer::Create(layout, Buffer::FromArray(vertices));
		auto indexBuffer = IndexBuffer::Create(Buffer::FromArray(indices));
		auto materialTable = Ref<MaterialTable>::Create();
		auto material = MaterialAsset::Create();
		material->UpdateMaterialIfDirty();
		materialTable->AddMaterial(0, material);

		return MeshSource::Create(vertexBuffer, indexBuffer, materialTable);
	}

	Ref<MeshSource> MeshFactory::GetCube()
	{
		if (s_MeshFactoryCache.CubeHandle == AssetHandle::Invalid)
		{
			auto meshSource = CreateCube();
			AssetManager::AddMemoryAsset(meshSource);
			s_MeshFactoryCache.CubeHandle = meshSource->Handle;
			return meshSource;
		}

		return AssetManager::GetAsset<MeshSource>(s_MeshFactoryCache.CubeHandle);
	}

	Ref<MeshSource> MeshFactory::GetSphere()
	{
		if (s_MeshFactoryCache.SphereHandle == AssetHandle::Invalid)
		{
			auto meshSource = CreateSphere();
			AssetManager::AddMemoryAsset(meshSource);
			s_MeshFactoryCache.SphereHandle = meshSource->Handle;
			return meshSource;
		}

		return AssetManager::GetAsset<MeshSource>(s_MeshFactoryCache.SphereHandle);
	}

}
