#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/EditorCamera.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/FrameBuffer.h"

namespace Shark {

	class Scene;

	class SceneRenderer : public RefCount
	{
	public:
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }

		void BeginScene(const DirectX::XMMATRIX& viewProj);
		void EndScene();

		void SubmitQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const DirectX::XMFLOAT4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void SubmitCirlce(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id = -1);


		void Resize(uint32_t width, uint32_t height);

		Ref<FrameBuffer> GetFinalFrameBuffer() const { return m_CompositPipeline->GetSpecification().TargetFrameBuffer; }
		Ref<Image2D> GetFinalImage() const { return m_CompositPipeline->GetSpecification().TargetFrameBuffer->GetImage(0); }

		Ref<FrameBuffer> GetFrameBuffer() const { return m_Renderer2DFrameBuffer; }
		Ref<Renderer2D> GetRenderer() const { return m_Renderer2D; }

	private:
		Ref<Scene> m_Scene;
		Ref<Renderer2D> m_Renderer2D;
		Ref<FrameBuffer> m_Renderer2DFrameBuffer;

		Ref<RenderCommandBuffer> m_CommandBuffer;
		
		Ref<Pipeline> m_CompositPipeline;

		bool m_NeedsResize = false;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	};

}
