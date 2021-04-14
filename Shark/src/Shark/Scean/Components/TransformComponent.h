#pragma once

#include <DirectXMath.h>

namespace Shark {

	struct TransformComponent
	{
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

}