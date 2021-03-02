#pragma once

#include "Shark/Scean/SceanCamera.h"

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
		~SpriteRendererComponent() = default;

		DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f};
	};

#if 0
	// For Later
	struct TextureComponent
	{
		TextureComponent() = default;
		TextureComponent(const Ref<Texture2D>& texture)
			: Texture(texture) {}
		~TextureComponent() = default;

		Ref<Texture2D> Texture;
	};
#endif

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

}