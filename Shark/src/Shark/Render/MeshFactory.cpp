#include "skpch.h"
#include "MeshFactory.h"

#include "Shark/Render/Renderer.h"

namespace Shark {

	Ref<Mesh> MeshFactory::CreateCube()
	{
#if 0
		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" },
		};

		glm::vec3 vertices[] = {
			{ -0.5f, -0.5f, -0.5f },
			{  0.5f, -0.5f, -0.5f },
			{ -0.5f,  0.5f, -0.5f },
			{  0.5f,  0.5f, -0.5f },
			{ -0.5f, -0.5f,  0.5f },
			{  0.5f, -0.5f,  0.5f },
			{ -0.5f,  0.5f,  0.5f },
			{  0.5f,  0.5f,  0.5f }
		};

		uint32_t indices[] = {
			0,2,1, 2,3,1,
			1,3,5, 3,7,5,
			2,6,3, 3,6,7,
			4,5,7, 4,7,6,
			0,4,2, 2,4,6,
			0,1,4, 1,5,4
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(layout, Buffer{ vertices, sizeof(vertices) });
		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(Buffer{ indices, sizeof(indices) });
		Ref<Material> material = Material::Create(Renderer::GetShaderLib()->Get("DefaultMeshShader"));

		return Mesh::Create(vertexBuffer, indexBuffer, material);
#endif
		return nullptr;
	}

}
