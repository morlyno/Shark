#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/EditorCamera.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Mesh.h"

namespace Shark {

	class Scene;

	struct SceneRendererSpecification
	{
		uint32_t Width = 0, Height = 0;
		bool IsSwapchainTarget = false;
		std::string DebugName;
	};

	class SceneRenderer : public RefCount
	{
	public:
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t VertexCount = 0;
			uint32_t IndexCount = 0;
		};

	public:
		SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName);
		SceneRenderer(Ref<Scene> scene, const SceneRendererSpecification& specification);
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }

		void BeginScene(const glm::mat4& viewProj, const glm::vec3& cameraPosition);
		void EndScene();

		void SubmitQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void SubmitFilledCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id = -1);
		void SubmitCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);

		void SubmitQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, bool isTransparent, int id);
		void SubmitFilledCircle(const glm::mat4& transform, float thickness, float fade, const glm::vec4& tintcolor, bool isTransparent, int id);
		void SubmitCircle(const glm::mat4& transform, const glm::vec4& tintcolor, int id);

		void SubmitText(const glm::mat4& transform, Ref<Font> font, const std::string& text, float kerning, float lineSpacing, const glm::vec4& color, int id);

		void SubmitPointLight(const glm::vec3& position, const glm::vec4& color, float intensity, const glm::vec3& radiance);
		void SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, int id);
		void SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, int id);

		void Resize(uint32_t width, uint32_t height);

		void DrawSettings();

		Ref<Image2D> GetFinalImage() const { return m_ExternalCompositeFrameBuffer->GetImage(0); }
		Ref<Image2D> GetIDImage() const { return m_GeometryFrameBuffer->GetImage(1); }
		Ref<FrameBuffer> GetExternalCompositFrameBuffer() const { return m_ExternalCompositeFrameBuffer; }

		Ref<Renderer2D> GetRenderer2D() const { return m_Renderer2D; }
		const Renderer2D::Statistics& GetRenderer2DStats() const { return m_Renderer2D->GetStatistics(); }

		const Statistics& GetStatisitcs() const { return m_Statistics; }

	private:
		void Initialize(const SceneRendererSpecification& specification);

	private:
		struct CBCamera
		{
			glm::mat4 ViewProj;
			glm::vec3 Position;
			float Padding;
		};

		struct CBLight
		{
			glm::vec4 Color;
			glm::vec3 Position;
			float Intensity;
			glm::vec3 Radiance;
			float Padding;
		};

		struct CBMeshData
		{
			glm::mat4 Transform;
			int ID;
			int padding1;
			int padding2;
			int padding3;
		};

	private:
		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;
		Statistics m_Statistics;

		Ref<ConstantBuffer> m_CBSceneData;
		Ref<ConstantBuffer> m_CBLight;

		uint32_t m_MeshTransformCBIndex = 0;
		std::vector<Ref<ConstantBuffer>> m_MeshTransformCBs;

		Ref<Renderer2D> m_Renderer2D;
		Ref<RenderCommandBuffer> m_CommandBuffer;
		Ref<GPUTimer> m_Timer;

		glm::mat4 m_ViewProjection;
		glm::vec3 m_CameraPosition;

		// Geometry
		Ref<FrameBuffer> m_GeometryFrameBuffer;
		Ref<FrameBuffer> m_ExternalCompositeFrameBuffer;

		Ref<Pipeline> m_MeshPipeline;

		bool m_NeedsResize = true;
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
	};

}
