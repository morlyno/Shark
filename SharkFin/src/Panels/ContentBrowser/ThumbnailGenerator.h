#pragma once

#include "Shark/Asset/Asset.h"
#include "Shark/Render/Texture.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Render/SceneRenderer.h"

namespace Shark {

	class AssetThumbnailGenerator
	{
	public:
		virtual ~AssetThumbnailGenerator() = default;
		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) = 0;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) = 0;
	};

	class MaterialThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MaterialThumbnailGenerator();
		~MaterialThumbnailGenerator();

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;

	private:
		AssetHandle m_SphereMesh;
		Entity m_Camera;
		Entity m_Sphere;
		Entity m_Light;
		Entity m_DirectionalLight;
	};

	class MeshThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MeshThumbnailGenerator();
		~MeshThumbnailGenerator();

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;

	private:
		Entity m_Camera;
		Entity m_Mesh;
		Entity m_DirectionalLight;
	};

	class MeshSourceThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		MeshSourceThumbnailGenerator();
		~MeshSourceThumbnailGenerator();

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;

	private:
		AssetHandle m_MeshHandle;
		Entity m_Camera;
		Entity m_Mesh;
		Entity m_DirectionalLight;
	};

	class SceneThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		SceneThumbnailGenerator();
		~SceneThumbnailGenerator();

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;

	private:
		Ref<Scene> m_BackupScene;
	};

	class EnvironmentThumbnailGenerator : public AssetThumbnailGenerator
	{
	public:
		EnvironmentThumbnailGenerator();
		~EnvironmentThumbnailGenerator();

		virtual void OnPrepare(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;
		virtual void OnFinish(AssetHandle assetHandle, Ref<Scene> scene, Ref<SceneRenderer> renderer) override;

	private:
		AssetHandle m_SphereHandle;
		AssetHandle m_MaterialHandle;
		Entity m_SkyEntity;
		Entity m_SphereEntity;
		Entity m_CameraEntity;
	};

	class ThumbnailGenerator : public RefCount
	{
	public:
		ThumbnailGenerator();

		Ref<Image2D> GenerateThumbnail(AssetHandle handle);

	private:
		Ref<Scene> m_Scene;
		Ref<SceneRenderer> m_Renderer;
		Ref<RenderCommandBuffer> m_CommandBuffer;

		bool m_Ready = false;

		Entity m_SkyLight;
		std::map<AssetType, Scope<AssetThumbnailGenerator>> m_AssetThumbnailGenerators;
	};

}
