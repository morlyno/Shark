#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/EditorCamera.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/StorageBuffer.h"

namespace Shark {

	class Scene;

	struct SceneRendererSpecification
	{
		uint32_t Width = 0, Height = 0;
		bool IsSwapchainTarget = false;
		std::string DebugName;
	};

	struct SceneRendererCamera
	{
		glm::mat4 View;
		glm::mat4 Projection;
		glm::vec3 Position;
	};

	class SceneRenderer : public RefCount
	{
	public:
		struct Statistics
		{
			TimeStep GPUTime;
			TimeStep GeometryPass;
			TimeStep SkyboxPass;

			uint32_t DrawCalls;
			uint32_t VertexCount;
			uint32_t IndexCount;
		};

	public:
		SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName);
		SceneRenderer(Ref<Scene> scene, const SceneRendererSpecification& specification);
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }

		void BeginScene(const SceneRendererCamera& camera);
		void EndScene();

		void SubmitQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor = 1.0f, const glm::vec4& tintcolor = { 1.0f, 1.0f, 1.0f, 1.0f }, int id = -1);
		void SubmitFilledCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id = -1);
		void SubmitCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id = -1);

		void SubmitQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id);
		void SubmitFilledCircle(const glm::mat4& transform, float thickness, float fade, const glm::vec4& tintcolor, int id);
		void SubmitCircle(const glm::mat4& transform, const glm::vec4& tintcolor, int id);

		void SubmitText(const glm::mat4& transform, Ref<Font> font, const std::string& text, float kerning, float lineSpacing, const glm::vec4& color, int id);

		void SubmitPointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float radius, float falloff);

		void SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, int id);
		void SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, int id);

		void Resize(uint32_t width, uint32_t height);

		Ref<Image2D> GetFinalImage() const { return m_ExternalCompositeFrameBuffer->GetImage(0); }
		Ref<Image2D> GetIDImage() const { return m_GeometryFrameBuffer->GetImage(1); }
		Ref<FrameBuffer> GetExternalCompositFrameBuffer() const { return m_ExternalCompositeFrameBuffer; }

		Ref<Renderer2D> GetRenderer2D() const { return m_Renderer2D; }
		const Renderer2D::Statistics& GetRenderer2DStats() const { return m_Renderer2D->GetStatistics(); }

		const Statistics& GetStatisitcs() const { return m_Statistics; }

	private:
		void ClearPass();
		void GeometryPass();
		void SkyboxPass();

	private:
		void Initialize(const SceneRendererSpecification& specification);

	private:
		struct CBScene
		{
			uint32_t LightCount = 0;
			float EnvironmentMapIntensity = 1;
			float Padding[2];
		};

		struct CBCamera
		{
			glm::mat4 ViewProj;
			glm::vec3 Position;
			float Padding;
		};

		struct CBSkybox
		{
			glm::mat4 SkyboxProjection;
		};

		struct CBSkyboxSettings
		{
			float Lod;
			float Padding[3];
		};

		struct Light
		{
			glm::vec3 Color;
			float P0;
			glm::vec3 Position;
			float P1;
			float Intensity;
			float Radius;
			float Falloff;
			float P2;
		};

		struct MeshPushConstant
		{
			glm::mat4 Transform;
			int ID;
		};

		struct MeshData
		{
			Ref<Mesh> Mesh;
			Ref<Material> Material;
			uint32_t SubmeshIndex;
			glm::mat4 Transform;
			int ID;
		};

	private:
		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;

		Statistics m_Statistics;
		PipelineStatistics m_PipelineStatistics;

		Ref<ConstantBuffer> m_CBScene;
		Ref<ConstantBuffer> m_CBCamera;
		Ref<ConstantBuffer> m_CBSkybox;
		Ref<ConstantBuffer> m_CBSkyboxSettings;
		Ref<StorageBuffer> m_SBLights;

		Ref<Renderer2D> m_Renderer2D;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		Ref<GPUPipelineQuery> m_PipelineQuery;
		Ref<GPUTimer> m_Timer;
		Ref<GPUTimer> m_GeometryPassTimer;
		Ref<GPUTimer> m_SkyboxPassTimer;

		glm::mat4 m_ViewProjection;
		glm::mat4 m_View;
		glm::mat4 m_Projection;
		glm::vec3 m_CameraPosition;

		std::vector<Light> m_Lights;
		std::vector<MeshData> m_Meshes;

		// Geometry
		Ref<FrameBuffer> m_GeometryFrameBuffer;
		Ref<FrameBuffer> m_ExternalCompositeFrameBuffer;

		Ref<RenderPass> m_PBRPass;

		bool m_NeedsResize = true;
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		float m_EnvironmentMapIntensity = 1.0f;
		float m_SkyboxLOD = 0;
		bool m_UpdateSkyboxSettings = false;

		Ref<TextureCube> m_EnvironmentMap;
		Ref<TextureCube> m_IrradianceMap;
		Ref<RenderPass> m_SkyboxPass;

		friend class SceneRendererPanel;
	};

}
