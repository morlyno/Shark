#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/EditorCamera.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/FrameBuffer.h"

namespace Shark {

	class Scene;

	struct SceneRendererOptions
	{
		bool ShowColliders = false;
		bool ShowCollidersOnTop = true;
	};

	class SceneRenderer : public RefCount
	{
	public:
		SceneRenderer(Ref<Scene> scene, const SceneRendererOptions& options = {});
		~SceneRenderer();

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }

		void BeginScene(const DirectX::XMMATRIX& viewProj);
		void EndScene();

		void SubmitQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void SubmitCirlce(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id = -1);

		void SubmitColliderBox(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scale);
		void SubmitColliderCirlce(const DirectX::XMFLOAT2& pos, float radius);

		void Resize(uint32_t width, uint32_t height);

		Ref<FrameBuffer> GetFinalFrameBuffer() const { return m_FinalFrameBuffer; }
		Ref<Image2D> GetFinalImage() const { return m_FinalFrameBuffer->GetImage(); }
		Ref<Image2D> GetIDImage() const { return m_GeometryFrameBuffer->GetImage(1); }

		Ref<Renderer2D> GetRenderer2D() const { return m_Renderer2D; }

		SceneRendererOptions& GetOptions() { return m_Options; }
		const SceneRendererOptions& GetOptions() const { return m_Options; }

	private:
		struct CBCamera
		{
			DirectX::XMMATRIX ViewProj;
		};

	private:
		Ref<Scene> m_Scene;

		Ref<Renderer2D> m_Renderer2D;
		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<ConstantBuffer> m_CameraConstantBuffer;

		// Geometry
		Ref<FrameBuffer> m_GeometryFrameBuffer;
		
		// Composit
		Ref<FrameBuffer> m_FinalFrameBuffer;

		bool m_NeedsResize = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		SceneRendererOptions m_Options;
	};

}
