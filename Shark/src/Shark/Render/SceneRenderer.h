#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include <set>
#include <span>

namespace Shark {
	class Scene;

	class Renderer2D;
	class RenderCommandBuffer;

	class Pipeline;
	class RenderPass;
	class FrameBuffer;
	class ConstantBuffer;
	class StorageBuffer;
	class Image2D;
	class Mesh;
	class MeshSource;
	class PBRMaterial;
}

namespace Shark {

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

	public:
		SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName);
		SceneRenderer(const SceneRendererSpecification& specification);
		SceneRenderer(Ref<Scene> scene);
		~SceneRenderer();

		void Resize(uint32_t width, uint32_t height);
		void SetClearColor(const glm::vec4& clearColor);

		void BeginScene(Ref<Scene> scene, const SceneRendererCamera& camera);
		void EndScene();

		void SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<PBRMaterial> material, const glm::mat4& transform, std::span<const glm::mat4> boneTransforms, UUID contextID, bool isSelected, int id);

		Ref<Renderer2D> GetRenderer2D() const;
		RefArg<Image2D> GetFinalPassImage() const;
		RefArg<Image2D> GetIDImage() const;
		RefArg<FrameBuffer> GetTargetFramebuffer() const;

		Options& GetOptions() { return m_Options; }
		const Statistics& GetStatisitcs() const { return m_Statistics; }
		//const Renderer2D::Statistics& GetRenderer2DStats() const { return m_Renderer2D->GetStatistics(); }

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
			uint32_t BoneBase;
			uint32_t BoneStride;
			float P0;
		};

		struct CBOutlineSettings
		{
			glm::vec3 Color;
			float PixelWidth;
			glm::vec2 TexelSize;
		};

		struct DrawCommand
		{
			Ref<Mesh> Mesh;
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<PBRMaterial> Material;
			glm::mat4 Transform;
			int ID;
			
			bool IsRigged = false;
			UUID ContextID;
		};

	private:
		Ref<Scene> m_Scene;
		SceneRendererSpecification m_Specification;

		Statistics m_Statistics;
		Options m_Options;

		Ref<ConstantBuffer> m_CBScene;
		Ref<ConstantBuffer> m_CBCamera;
		Ref<ConstantBuffer> m_CBSkybox;
		Ref<ConstantBuffer> m_CBSkyboxSettings;
		Ref<ConstantBuffer> m_CBCompositeSettings;
		Ref<StorageBuffer> m_SBPointLights;
		Ref<StorageBuffer> m_SBDirectionalLights;
		Ref<ConstantBuffer> m_CBOutlineSettings;
		Ref<StorageBuffer> m_SBBoneTransforms;

		Ref<Renderer2D> m_Renderer2D;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		glm::mat4 m_ViewProjection;
		glm::mat4 m_View;
		glm::mat4 m_Projection;
		glm::vec3 m_CameraPosition;

		std::vector<DrawCommand> m_DrawList;
		std::vector<DrawCommand> m_SelectedDrawList;
		std::set<Ref<PBRMaterial>> m_MaterialsToUpdate;

		struct BoneTransformData
		{
			std::vector<glm::mat4> Transforms;
			size_t BaseIndex = 0;
			size_t Stride = 0;
		};

		size_t m_BoneTransformBufferSize = 0;
		std::vector<glm::mat4> m_BoneTransformsUploadBuffer[2];
		std::unordered_map<UUID, BoneTransformData> m_MeshBoneTransforms;

		Ref<RenderPass> m_GeometryPass;
		Ref<RenderPass> m_GeometryAnimatedPass;
		Ref<RenderPass> m_SelectedGeometryPass;
		Ref<RenderPass> m_SelectedGeometryAnimatedPass;
		Ref<RenderPass> m_SkyboxPass;
		Ref<RenderPass> m_CompositePass;

		Ref<Pipeline> m_GeometryPipeline;
		Ref<Pipeline> m_GeometryAnimatedPipeline;
		Ref<Pipeline> m_SelectedGeometryPipeline;
		Ref<Pipeline> m_SelectedGeometryAnimatedPipeline;
		Ref<Pipeline> m_SkyboxPipeline;
		Ref<Pipeline> m_CompositePipeline;

		Ref<RenderPass> m_JumpFloodInitPass;
		Ref<RenderPass> m_JumpFloodPass[2];
		Ref<RenderPass> m_JumpFloodCompositePass;
		Ref<Pipeline> m_JumpFloodInitPipeline;
		Ref<Pipeline> m_JumpFloodPipeline;
		Ref<Pipeline> m_JumpFloodCompositePipeline;

		std::vector<Ref<FrameBuffer>> m_TempFramebuffers;

		bool m_NeedsResize = true;
		glm::vec4 m_ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };

		float m_OutlinePixelWidth = 4.5;
		int m_JumpFloodSteps = 3;
		glm::vec4 m_OutlineColor = { 0.3f, 0.1f, 0.7f, 1.0f };

		friend class SceneRendererPanel;
	};

}
