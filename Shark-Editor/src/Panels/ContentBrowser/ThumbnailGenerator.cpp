#include "ThumbnailGenerator.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Serialization/Import/TextureImporter.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/Font.h"

namespace Shark {

	namespace utils {

		static Entity CreateMeshCamera(Ref<Scene> scene, const AABB& aabb, float cameraDistance = 1.5f)
		{
			Entity cameraEntity = scene->CreateEntity("Camera");
			auto& cameraComp = cameraEntity.AddComponent<CameraComponent>(true);

			auto& transform = cameraEntity.Transform();
			transform.Translation = { -5.0f, 5.0f, -5.0f };
			transform.Rotation = { glm::radians(27.0f), glm::radians(40.0f), 0 };

			glm::vec3 forward = glm::quat(transform.Rotation) * glm::vec3(0, 0, 1);
			glm::vec3 objectSizes = aabb.Size();
			float objectSize = glm::max(glm::max(objectSizes.x, objectSizes.y), objectSizes.z);
			float cameraView = 2.0f * glm::tan(0.5f * cameraComp.PerspectiveFOV);
			float distance = cameraDistance * objectSize / cameraView;
			distance += objectSize * 0.5f;

			glm::vec3 translation = aabb.Center() - distance * forward;
			transform.Translation = translation;

			float planeDistance = (objectSize / cameraView + objectSize * 0.5f);
			cameraComp.Near = glm::distance(transform.Translation, aabb.Center() - planeDistance * forward);
			cameraComp.Far = glm::distance(transform.Translation, aabb.Center() + planeDistance * forward);
			
			cameraComp.Recalculate();
			return cameraEntity;
		}

		static Entity CreateDefaultCamera(Ref<Scene> scene, float cameraDistance = 1.75f)
		{
			Entity entity = scene->CreateEntity("Camera");
			entity.AddComponent<CameraComponent>();
			entity.Transform().Translation.z = -cameraDistance;
			return entity;
		}

	}

	void AssetThumbnailGenerator::OnRender(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		scene->OnRenderRuntime(renderer);
	}

	Ref<Image2D> AssetThumbnailGenerator::GetImage(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		return renderer->GetFinalPassImage();
	}

	//////////////////////////////////////////////////////////////////
	///// Material

	MaterialThumbnailGenerator::MaterialThumbnailGenerator()
	{
		AssetHandle meshSource = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Meshes/Default/Sphere.gltf");
		m_SphereMesh = AssetManager::CreateMemoryOnlyAsset<Mesh>(meshSource);
	}

	MaterialThumbnailGenerator::~MaterialThumbnailGenerator()
	{
		AssetManager::DeleteMemoryAsset(m_SphereMesh);
	}

	void MaterialThumbnailGenerator::OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		m_Camera = utils::CreateDefaultCamera(scene);
		m_Sphere = scene->CreateEntity("Sphere");
		const auto& meshComponent = m_Sphere.AddComponent<StaticMeshComponent>(m_SphereMesh);
		meshComponent.MaterialTable->SetMaterial(0, assetHandle);

		//m_Light = scene->CreateEntity("Light");
		//m_Light.AddComponent<PointLightComponent>();
		//m_Light.Transform().Translation = { -3.0f, 2.0f, -3.0f };

		m_DirectionalLight = scene->CreateEntity("Directional Light");
		m_DirectionalLight.Transform().Rotation = glm::vec3(glm::radians(-68.0f), glm::radians(-68.0f), 0.0f);
		m_DirectionalLight.AddComponent<DirectionalLightComponent>().Intensity = 1.0f;
	}

	void MaterialThumbnailGenerator::OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		scene->DestroyEntity(m_Camera);
		scene->DestroyEntity(m_Sphere);
		//scene->DestroyEntity(m_Light);
		scene->DestroyEntity(m_DirectionalLight);
	}

	//////////////////////////////////////////////////////////////////
	///// Mesh

	MeshThumbnailGenerator::MeshThumbnailGenerator()
	{
	}

	MeshThumbnailGenerator::~MeshThumbnailGenerator()
	{
	}

	void MeshThumbnailGenerator::OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(assetHandle);
		m_Mesh = scene->InstantiateMesh(mesh);

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());
		if (!meshSource)
			m_Camera = utils::CreateDefaultCamera(scene);
		else
			m_Camera = utils::CreateMeshCamera(scene, meshSource->GetBoundingBox());

		m_DirectionalLight = scene->CreateEntity("Directional Light");
		m_DirectionalLight.Transform().Rotation = glm::vec3(glm::radians(-58.0f), glm::radians(-40.0f), 0.0f);
		m_DirectionalLight.AddComponent<DirectionalLightComponent>().Intensity = 1.0f;
	}

	void MeshThumbnailGenerator::OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		scene->DestroyEntity(m_Mesh);
		scene->DestroyEntity(m_Camera);
		scene->DestroyEntity(m_DirectionalLight);
	}

	//////////////////////////////////////////////////////////////////
	///// MaterialSource

	MeshSourceThumbnailGenerator::MeshSourceThumbnailGenerator()
	{

	}

	MeshSourceThumbnailGenerator::~MeshSourceThumbnailGenerator()
	{

	}

	void MeshSourceThumbnailGenerator::OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		m_MeshHandle = AssetManager::CreateMemoryOnlyAsset<Mesh>(assetHandle);
		Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(m_MeshHandle);
		m_Mesh = scene->InstantiateMesh(mesh);

		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(assetHandle);
		m_Camera = utils::CreateMeshCamera(scene, meshSource->GetBoundingBox());

		m_DirectionalLight = scene->CreateEntity("Directional Light");
		m_DirectionalLight.Transform().Rotation = glm::vec3(glm::radians(-58.0f), glm::radians(-40.0f), 0.0f);
		m_DirectionalLight.AddComponent<DirectionalLightComponent>().Intensity = 1.0f;
	}

	void MeshSourceThumbnailGenerator::OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		scene->DestroyEntity(m_Mesh);
		scene->DestroyEntity(m_Camera);
		scene->DestroyEntity(m_DirectionalLight);
		AssetManager::DeleteMemoryAsset(m_MeshHandle);
	}

	//////////////////////////////////////////////////////////////////
	///// Scene

	SceneThumbnailGenerator::SceneThumbnailGenerator()
	{
	}

	SceneThumbnailGenerator::~SceneThumbnailGenerator()
	{

	}

	void SceneThumbnailGenerator::OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		// TODO(moro): maybe copy the scene
		Ref<Scene> assetScene = AssetManager::GetAsset<Scene>(assetHandle);
		assetScene->SetViewportSize(scene->GetViewportWidth(), scene->GetViewportHeight());

		if (!assetScene->HasActiveCamera())
			m_CameraEntity = utils::CreateDefaultCamera(assetScene);
	}

	void SceneThumbnailGenerator::OnRender(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		Ref<Scene> assetScene = AssetManager::GetAsset<Scene>(assetHandle);
		assetScene->OnRenderRuntime(renderer);
	}

	void SceneThumbnailGenerator::OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		if (m_CameraEntity)
		{
			Ref<Scene> assetScene = AssetManager::GetAsset<Scene>(assetHandle);
			assetScene->DestroyEntity(m_CameraEntity);
		}
	}

	//////////////////////////////////////////////////////////////////
	///// Environment

	EnvironmentThumbnailGenerator::EnvironmentThumbnailGenerator()
	{
		AssetHandle meshSource = Project::GetActiveEditorAssetManager()->GetEditorAsset("Resources/Meshes/Default/Sphere.gltf");
		m_SphereHandle = AssetManager::CreateMemoryOnlyAsset<Mesh>(meshSource);

		m_MaterialHandle = AssetManager::CreateMemoryOnlyAsset<MaterialAsset>();
		Ref<MaterialAsset> reflectiveMaterial = AssetManager::GetAsset<MaterialAsset>(m_MaterialHandle);
		reflectiveMaterial->SetMetalness(1.0f);
		reflectiveMaterial->SetRoughness(0.0f);
	}

	EnvironmentThumbnailGenerator::~EnvironmentThumbnailGenerator()
	{
		AssetManager::DeleteMemoryAsset(m_SphereHandle);
		AssetManager::DeleteMemoryAsset(m_MaterialHandle);
	}

	void EnvironmentThumbnailGenerator::OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		m_SkyEntity = scene->CreateEntity("Sky");
		m_SkyEntity.AddComponent<SkyComponent>(assetHandle);

		m_SphereEntity = scene->CreateEntity("Sphere");
		auto& smc = m_SphereEntity.AddComponent<StaticMeshComponent>(m_SphereHandle);
		smc.MaterialTable->SetMaterial(0, m_MaterialHandle);

		m_CameraEntity = utils::CreateDefaultCamera(scene);
	}

	void EnvironmentThumbnailGenerator::OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		scene->DestroyEntity(m_SkyEntity);
		scene->DestroyEntity(m_SphereEntity);
		scene->DestroyEntity(m_CameraEntity);
	}

	//////////////////////////////////////////////////////////////////
	///// Texture

	TextureThumbnailGenerator::TextureThumbnailGenerator()
	{
	}

	TextureThumbnailGenerator::~TextureThumbnailGenerator()
	{

	}

	Ref<Image2D> TextureThumbnailGenerator::GetImage(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer)
	{
		auto texture = AssetManager::GetAsset<Texture2D>(assetHandle);
		return texture->GetImage();
	}

	ThumbnailGenerator::ThumbnailGenerator()
	{
		m_Scene = Ref<Scene>::Create("Thumbnail Generator");
		m_Scene->SetViewportSize(512, 512);

		SceneRendererSpecification specification;
		specification.Width = 512;
		specification.Height = 512;
		specification.DebugName = "Thumbnail Generator";
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene, specification);

		m_AssetThumbnailGenerators[AssetType::Material] = Scope<MaterialThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::Mesh] = Scope<MeshThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::MeshSource] = Scope<MeshSourceThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::Scene] = Scope<SceneThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::Environment] = Scope<EnvironmentThumbnailGenerator>::Create();
		m_AssetThumbnailGenerators[AssetType::Texture] = Scope<TextureThumbnailGenerator>::Create();

		m_SkyLight = m_Scene->CreateEntity("SkyLight");
		auto& skyLight = m_SkyLight.AddComponent<SkyComponent>();

		auto [radianceMap, irradianceMap] = Renderer::CreateEnvironmentMap("Resources/Environment/green_point_park_4k.hdr");
		skyLight.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(radianceMap, irradianceMap);
		skyLight.Intensity = 0.8f;
		skyLight.Lod = (float)(AssetManager::GetAsset<Environment>(skyLight.SceneEnvironment)->GetRadianceMap()->GetMipLevelCount() - 1);

		m_CommandBuffer = RenderCommandBuffer::Create();
	}

	Ref<Image2D> ThumbnailGenerator::GenerateThumbnail(AssetHandle handle)
	{
		if (!AssetManager::IsValidAssetHandle(handle))
			return nullptr;

		AssetType assetType = AssetManager::GetAssetType(handle);

		auto& generator = m_AssetThumbnailGenerators.at(assetType);
		generator->OnPrepare(handle, m_Scene, m_Renderer);
		generator->OnRender(handle, m_Scene, m_Renderer);
		generator->OnFinish(handle, m_Scene, m_Renderer);

		auto generated = generator->GetImage(handle, m_Scene, m_Renderer);

		ImageSpecification specification;
		specification.Width = 512;
		specification.Height = 512;
		specification.Format = ImageFormat::RGBA8UNorm;
		auto result = Image2D::Create(specification);

		m_CommandBuffer->Begin();
		Renderer::BlitImage(m_CommandBuffer, generated, result);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		return result;
	}

}
