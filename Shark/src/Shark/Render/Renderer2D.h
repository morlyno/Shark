#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/Texture.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components/TransformComponent.h"

//#define SK_OLD_RENDERER2D

namespace Shark {

	class Renderer2D
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginScene(Camera& camera, const DirectX::XMMATRIX& view);
		static void BeginScene(EditorCamera& camera);
		static void EndScene();

		template<typename Function>
		static void Submit(const Function& function) { AddCallbackFunction(std::function<void()>(function)); }

		static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id = -1);


		static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color, int id = -1);
		static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id = -1);

		static void DrawEntity(Entity entity);
		static void DrawTransform(const TransformComponent& transform, const DirectX::XMFLOAT4& color, int id = -1);

		struct Statistics
		{
			uint32_t DrawCalls;
			uint32_t DrawCommands;
			uint32_t ElementCount;
			uint32_t VertexCount;
			uint32_t IndexCount;
			uint32_t TextureCount;
			uint32_t Callbacks;
		};
		static Statistics GetStatistics();

	private:
		static void AddCallbackFunction(const std::function<void()>& func);
	};

}