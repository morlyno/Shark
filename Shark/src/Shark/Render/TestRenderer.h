#pragma once

#include "Shark/Render/Camera.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/Texture.h"

#include "Shark/Scean/Entity.h"
#include "Shark/Render/Material.h"

namespace Shark {

	class TestRenderer
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginScean(Camera& camera, const DirectX::XMMATRIX& view);
		static void BeginScean(EditorCamera& camera);
		static void EndScean();

		static void DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		static void DrawRotatedQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id = -1);

		static void DrawEntity(Entity entity);

		static void DrawQuad(const Ref<Material>& material);

		static Ref<Material> GetMaterial();
	};

}
