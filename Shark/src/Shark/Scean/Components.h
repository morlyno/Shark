#pragma once

#include "Shark/Scean/SceanCamera.h"
#include "Shark/Render/Texture.h"
#include "Shark/Scean/NativeScript.h"

#include <DirectXMath.h>

namespace Shark {

	struct TagComponent
	{
		TagComponent() = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
		~TagComponent() = default;

		std::string Tag;
	};

	struct TransformComponent
	{
		TransformComponent() = default;
		TransformComponent(const DirectX::XMFLOAT3& position)
			: Position(position) {}
		TransformComponent(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling)
			: Position(position), Rotation(rotation), Scaling(scaling) {}
		~TransformComponent() = default;

		DirectX::XMMATRIX GetTranform() const
		{
			return DirectX::XMMatrixScaling(Scaling.x, Scaling.y, Scaling.z) *
				DirectX::XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z) *
				DirectX::XMMatrixTranslation(Position.x, Position.y, Position.z);
		}

		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 Rotation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 Scaling = { 1.0f, 1.0f, 1.0f };
	};

	struct SpriteRendererComponent
	{
		SpriteRendererComponent() = default;
		SpriteRendererComponent(const DirectX::XMFLOAT4& color)
			: Color(color) {}
		SpriteRendererComponent(const Ref<Texture2D>& texture)
			: Texture(texture) {}
		SpriteRendererComponent(const DirectX::XMFLOAT4& color, const Ref<Texture2D>& texture, float tilingfactor)
			: Color(color), Texture(texture), TilingFactor(tilingfactor) {}
		~SpriteRendererComponent() = default;

		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f};
		Ref<Texture2D> Texture;
		float TilingFactor = 1.0f;
	};

	struct CameraComponent
	{
		CameraComponent() = default;
		CameraComponent(const Camera& camera)
			: Camera(camera) {}
		CameraComponent(const SceanCamera& camera)
			: Camera(camera) {}
		CameraComponent(const DirectX::XMMATRIX& projection)
			: Camera(projection) {}
		~CameraComponent() = default;

		SceanCamera Camera;
	};

	struct NativeScriptComponent
	{
		NativeScript* Script;

		NativeScript* (*CreateScript)(Entity entity);
		void (*DestroyScript)(NativeScript* ns);

		template<typename Type>
		void Bind()
		{
			CreateScript = [](Entity entity) { NativeScript* s = reinterpret_cast<NativeScript*>(new Type()); s->SetEntity(entity); return s; };
			DestroyScript = [](NativeScript* ns) { delete ns; ns = nullptr; };
		}
	};

}