#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Asset/AssetTypes.h"

#include "Shark/Render/Camera.h"
#include "Shark/Render/MaterialTable.h"

namespace Shark {

	///////////////////////////////////////////////////////////////////////////
	//// Sprites and Text /////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	struct SpriteRendererComponent
	{
		glm::vec4 Color           = glm::vec4(1.0f);
		AssetHandle TextureHandle = AssetHandle::Invalid;
		glm::vec2 TilingFactor    = glm::vec2(1.0f);
		bool Transparent          = false;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color  = glm::vec4(1.0f);
		float Thickness  = 1.0f;
		float Fade       = 0.002f;
		bool Filled      = true;
		bool Transparent = false;
	};

	struct TextRendererComponent
	{
		AssetHandle FontHandle = AssetHandle::Invalid;
		std::string Text       = {};
		glm::vec4 Color        = glm::vec4(1.0f);
		float Kerning          = 0.0f;
		float LineSpacing      = 0.0f;
	};

	///////////////////////////////////////////////////////////////////////////
	//// Meshes ///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	struct MeshComponent
	{
		AssetHandle Mesh;
		std::vector<UUID> BoneEntityIDs;
	};

	struct MeshFilterComponent
	{
		UUID RootEntityID;
	};

	struct SubmeshComponent
	{
		AssetHandle Mesh;
		uint32_t SubmeshIndex = 0;
		AssetHandle Material;
		bool Visible = true;

		SubmeshComponent() = default;
		SubmeshComponent(AssetHandle mesh, uint32_t submeshIndex)
			: Mesh(mesh), SubmeshIndex(submeshIndex) {
		}
	};

	struct StaticMeshComponent
	{
		AssetHandle StaticMesh;
		Ref<MaterialTable> MaterialTable = Ref<Shark::MaterialTable>::Create();
		bool Visible = true;

		StaticMeshComponent() = default;
		StaticMeshComponent(AssetHandle staticMesh)
			: StaticMesh(staticMesh) {
		}
	};

	///////////////////////////////////////////////////////////////////////////
	//// Lights and Sky ///////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	struct PointLightComponent
	{
		glm::vec4 Radiance = glm::vec4(1.0f);
		float Intensity = 1.0f;
		float Radius = 10.0f;
		float Falloff = 1.0f;
	};

	struct DirectionalLightComponent
	{
		glm::vec4 Radiance = glm::vec4(1.0f);
		float Intensity = 1.0f;
	};

	struct SkyComponent
	{
		AssetHandle SceneEnvironment = AssetHandle::Invalid;
		bool DynamicSky = false;
		float Intensity = 1.0f;
		float Lod = 0.0f;

		SkyComponent() = default;
		SkyComponent(AssetHandle environment)
			: SceneEnvironment(environment) {
		}
	};

	///////////////////////////////////////////////////////////////////////////
	//// Camera ///////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////

	struct CameraComponent
	{
		bool IsPerspective = true;
		float AspectRatio = 16.0f / 9.0f;
		float OrthographicSize = 10.0f;
		float PerspectiveFOV = 0.785398f;
		float Near = 0.3f;
		float Far = 1000.0f;

		Camera Camera;

		const glm::mat4& GetProjection() const { return Camera.GetProjection(); }
		void SetProjection(const glm::mat4& projection) { Camera.SetProjectionMatrix(projection); }

		void RecalculatePerspective() { Camera.SetPerspectiveProjectionMatrix(PerspectiveFOV, AspectRatio, Near, Far); }
		void RecalculateOrthographic() { SetProjection(glm::ortho(-OrthographicSize * AspectRatio, OrthographicSize * AspectRatio, OrthographicSize, OrthographicSize, Near, Far)); }
		void Recalculate() { IsPerspective ? RecalculatePerspective() : RecalculateOrthographic(); }

		CameraComponent() = default;
		CameraComponent(bool perspective) : IsPerspective(perspective) {}
		CameraComponent(bool perspective, float aspectRatio) : IsPerspective(perspective), AspectRatio(aspectRatio) {}
	};

}
