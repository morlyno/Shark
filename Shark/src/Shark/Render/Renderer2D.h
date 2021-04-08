#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/Texture.h"

#include "Shark/Scean/Entity.h"
#include "Shark/Scean/Components/TransformComponent.h"

//#define SK_OLD_RENDERER2D

namespace Shark {

	class Renderer2D
	{
	public:
		static void Init();
		static void ShutDown();

		static void BeginScean(Camera& camera, const DirectX::XMMATRIX& view);
		static void BeginScean(EditorCamera& camera);
		static void EndScean();

		template<typename Function>
		static void Submit(const Function& function) { AddCallbackFunction(std::function<void()>(function)); }

		static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);


		static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
		static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);

		static void DrawEntity(Entity entity);
		static void DrawTransform(const TransformComponent& transform, const DirectX::XMFLOAT4& color);

	private:
		static void AddCallbackFunction(const std::function<void()>& func);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SK_OLD_RENDERER2D

	namespace Old {

		class Renderer2D
		{
		public:
			static void Init();
			static void ShutDown();

			static void BeginScean(Camera& camera, const DirectX::XMMATRIX& view);
			static void BeginScean(EditorCamera& camera);
			static void EndScean();


			// Quad
			static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
			static void DrawQuad(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);

			static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
			static void DrawRotatedQuad(const DirectX::XMFLOAT2& position, float rotation, const DirectX::XMFLOAT2& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);


			// Circle
			static void DrawCircle(const DirectX::XMFLOAT2& center, float radius, const DirectX::XMFLOAT4& color);
			static void DrawCircle(const DirectX::XMFLOAT2& center, float radius, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);

			static void DrawRotatedCircle(const DirectX::XMFLOAT2& center, float radius, float rotation, const DirectX::XMFLOAT4& color);
			static void DrawRotatedCircle(const DirectX::XMFLOAT2& center, float radius, float rotation, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor);

			// Entity
			static void DrawEntity(Entity entity);


			struct BatchStatistics
			{
				uint32_t DrawCalls;
				uint32_t QuadCount;
				uint32_t QuadTextureCount;
				uint32_t CircleCount;
				uint32_t CircleTextureCount;

				uint32_t QuadVertexCount() const { return QuadCount * 4; }
				uint32_t QuadIndexCount() const { return QuadCount * 6; }
				uint32_t CircleVertexCount() const { return CircleCount * 4; }
				uint32_t CircleIndexCount() const { return CircleCount * 6; }

				uint32_t GeometryCount() const { return QuadCount + CircleCount; }
				uint32_t TextureCount() const { return QuadTextureCount + CircleTextureCount; }
				uint32_t VertexCount() const { return (QuadCount + CircleCount) * 4; }
				uint32_t IndexCount() const { return (QuadCount + CircleCount) * 6; }
			};
		public:
			static BatchStatistics GetBatchStatistics();
		};

	}

#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#ifdef SK_OLD_RENDERER2D
//
//	namespace Old {
//
//		class Renderer2D
//		{
//		public:
//			static void Init();
//			static void ShutDown();
//
//			static void BeginScean(Camera& camera, const DirectX::XMMATRIX& view);
//			static void BeginScean(EditorCamera& camera);
//			static void EndScean();
//
//			static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
//			static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
//			static void DrawQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor = 1, const DirectX::XMFLOAT4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f });
//			static void DrawQuad(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor = 1, const DirectX::XMFLOAT4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f });
//
//			static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
//			static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, const DirectX::XMFLOAT4& color);
//			static void DrawRotatedQuad(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor = 1, const DirectX::XMFLOAT4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f });
//			static void DrawRotatedQuad(const DirectX::XMFLOAT3& pos, float rotation, const DirectX::XMFLOAT2& scaling, Ref<Texture2D>& texture, float tilingfactor = 1, const DirectX::XMFLOAT4& tint_color = { 1.0f, 1.0f, 1.0f, 1.0f });
//
//			static void DrawQuad(const DirectX::XMMATRIX& translation, const DirectX::XMFLOAT4& color);
//			static void DrawQuad(const DirectX::XMMATRIX& translation, Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tint_color);
//
//			static void DrawEntity(Entity entity);
//
//			struct Statistiks
//			{
//				uint32_t DrawCalls = 0;
//				uint32_t QuadCount = 0;
//				uint32_t TextureCount = 0;
//
//				uint32_t VertexCount() const { return QuadCount * 4; }
//				uint32_t IndexCount() const { return QuadCount * 6; }
//			};
//			static Statistiks GetStats();
//		private:
//			static void ResetStats();
//		};
//
//	}
//
//#endif

}