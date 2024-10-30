#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Render/EditorCamera.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/RenderCommandBuffer.h"
#include "Shark/Render/Pipeline.h"
#include "Shark/Render/FrameBuffer.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/StorageBuffer.h"
#include "Shark/Render/Environment.h"

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
			TimeStep GPUTime = 0;
			TimeStep GeometryPass = 0;
			TimeStep SkyboxPass = 0;
			TimeStep CompositePass = 0;
			TimeStep JumpFloodPass = 0;

			uint32_t DrawCalls = 0;
			uint32_t VertexCount = 0;
			uint32_t IndexCount = 0;
		};

		struct Options
		{
			bool JumpFlood = true;
			bool Tonemap = true;
			bool GammaCorrect = true;
			float Exposure = 1.0f;
		};

		struct TimestampQueries
		{
			uint32_t TotalTimeQuery = (uint32_t)-1;
			uint32_t GeometryPassQuery = (uint32_t)-1;
			uint32_t SkyboxPassQuery = (uint32_t)-1;
			uint32_t CompositePassQuery = (uint32_t)-1;
			uint32_t JumpFloodPassQuery = (uint32_t)-1;
		};

	public:
		SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName);
		SceneRenderer(Ref<Scene> scene, const SceneRendererSpecification& specification);
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void Resize(uint32_t width, uint32_t height);
		void SetClearColor(const glm::vec4& clearColor);

		void SetScene(Ref<Scene> scene) { m_Scene = scene; }
		void BeginScene(const SceneRendererCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialAsset> material, const glm::mat4& transform, int id);
		void SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialAsset> material, const glm::mat4& transform);

		Ref<Renderer2D> GetRenderer2D() const { return m_Renderer2D; }
		Ref<Image2D> GetFinalPassImage() const { return m_CompositePass->GetOutput(0); }
		Ref<Image2D> GetIDImage() const { return m_CompositePass->GetOutput(1); }
		Ref<FrameBuffer> GetTargetFramebuffer() const { return m_CompositePass->GetTargetFramebuffer(); }

		Options& GetOptions() { return m_Options; }
		const Statistics& GetStatisitcs() const { return m_Statistics; }
		const Renderer2D::Statistics& GetRenderer2DStats() const { return m_Renderer2D->GetStatistics(); }

		uint32_t GetViewportWidth() const { return m_Specification.Width; }
		uint32_t GetViewportHeight() const { return m_Specification.Height; }

	private:
		void PreRender();

		void GeometryPass();
		void SkyboxPass();
		void JumpFloodPass();

	private:
		void Initialize(const SceneRendererSpecification& specification);

	private:
		struct CBScene
		{
			uint32_t PointLightCount = 0;
			uint32_t DirectionalLightCount = 0;
			float EnvironmentMapIntensity = 1.0f;
			float P0;
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
			float Intensity;
			float P0, P1;
		};

		struct CBCompositeSettings
		{
			uint32_t Tonemap = 0;
			uint32_t GammaCorrect = 0;
			float Exposure = 1.0f;
			float P0;
		};

		struct MeshPushConstant
		{
			glm::mat4 Transform;
			int ID;
		};

		struct CBOutlineSettings
		{
			glm::vec4 Color;
		};

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> Material;
			glm::mat4 Transform;
			int ID;
		};

	private:
		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;

		Statistics m_Statistics;
		PipelineStatistics m_PipelineStatistics;
		Options m_Options;

		Ref<ConstantBuffer> m_CBScene;
		Ref<ConstantBuffer> m_CBCamera;
		Ref<ConstantBuffer> m_CBSkybox;
		Ref<ConstantBuffer> m_CBSkyboxSettings;
		Ref<ConstantBuffer> m_CBCompositeSettings;
		Ref<StorageBuffer> m_SBPointLights;
		Ref<StorageBuffer> m_SBDirectionalLights;
		Ref<ConstantBuffer> m_CBOutlineSettings;

		Ref<Renderer2D> m_Renderer2D;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		TimestampQueries m_TimestampQueries;

		glm::mat4 m_ViewProjection;
		glm::mat4 m_View;
		glm::mat4 m_Projection;
		glm::vec3 m_CameraPosition;

		std::vector<DrawCommand> m_DrawList;
		std::vector<DrawCommand> m_SelectedDrawList;

		Ref<RenderPass> m_GeometryPass;
		Ref<RenderPass> m_SelectedGeometryPass;
		Ref<RenderPass> m_SkyboxPass;
		Ref<RenderPass> m_CompositePass;

		Ref<Material> m_SelectedGeometryMaterial;

		Ref<RenderPass> m_JumpFloodInitPass;
		Ref<RenderPass> m_JumpFloodPass[2];
		Ref<RenderPass> m_JumpFloodCompositePass;
		Ref<Material> m_JumpFloodInitMaterial;
		Ref<Material> m_JumpFloodPassMaterial[2];
		Ref<Material> m_JumpFloodCompositeMaterial;

		std::vector<Ref<FrameBuffer>> m_TempFramebuffers;



		bool m_NeedsResize = true;
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		float m_OutlinePixelWidth = 4.5;
		int m_JumpFloodSteps = 3;
		glm::vec4 m_OutlineColor = { 0.3f, 0.1f, 0.7f, 1.0f };

		friend class SceneRendererPanel;
	};

}
