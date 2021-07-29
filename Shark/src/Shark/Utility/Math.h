#pragma once

#include <DirectXMath.h>

namespace Shark {

	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3& operator+=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3& operator-=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);

}

namespace Shark::Math {

	template<typename T>
	T pow2(T val)
	{
		return pow(val, 2);
	}

	DirectX::XMFLOAT3 GetRotation(DirectX::XMMATRIX matrix);
	DirectX::XMFLOAT3 GetRotation(const DirectX::XMFLOAT4X4& matrix);

}
