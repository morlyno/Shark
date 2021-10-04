#pragma once

#include <DirectXMath.h>

namespace Shark {

	DirectX::XMFLOAT3 operator+(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3& operator+=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3 operator-(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);
	DirectX::XMFLOAT3& operator-=(DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs);


	DirectX::XMVECTOR    XM_CALLCONV     operator+ (DirectX::FXMVECTOR V);
	DirectX::XMVECTOR    XM_CALLCONV     operator- (DirectX::FXMVECTOR V);

	DirectX::XMVECTOR& XM_CALLCONV     operator+= (DirectX::XMVECTOR& V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR& XM_CALLCONV     operator-= (DirectX::XMVECTOR& V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR& XM_CALLCONV     operator*= (DirectX::XMVECTOR& V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR& XM_CALLCONV     operator/= (DirectX::XMVECTOR& V1, DirectX::FXMVECTOR V2);

	DirectX::XMVECTOR& operator*= (DirectX::XMVECTOR& V, float S);
	DirectX::XMVECTOR& operator/= (DirectX::XMVECTOR& V, float S);

	DirectX::XMVECTOR    XM_CALLCONV     operator+ (DirectX::FXMVECTOR V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR    XM_CALLCONV     operator- (DirectX::FXMVECTOR V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR    XM_CALLCONV     operator* (DirectX::FXMVECTOR V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR    XM_CALLCONV     operator/ (DirectX::FXMVECTOR V1, DirectX::FXMVECTOR V2);
	DirectX::XMVECTOR    XM_CALLCONV     operator* (DirectX::FXMVECTOR V, float S);
	DirectX::XMVECTOR    XM_CALLCONV     operator* (float S, DirectX::FXMVECTOR V);
	DirectX::XMVECTOR    XM_CALLCONV     operator/ (DirectX::FXMVECTOR V, float S);

	namespace DXMath {

		inline float             Store1(const DirectX::XMVECTOR& v) { float f1;             DirectX::XMStoreFloat(&f1, v);  return f1; }
		inline DirectX::XMFLOAT2 Store2(const DirectX::XMVECTOR& v) { DirectX::XMFLOAT2 f2; DirectX::XMStoreFloat2(&f2, v); return f2; }
		inline DirectX::XMFLOAT3 Store3(const DirectX::XMVECTOR& v) { DirectX::XMFLOAT3 f3; DirectX::XMStoreFloat3(&f3, v); return f3; }
		inline DirectX::XMFLOAT4 Store4(const DirectX::XMVECTOR& v) { DirectX::XMFLOAT4 f4; DirectX::XMStoreFloat4(&f4, v); return f4; }

		inline DirectX::XMVECTOR Load(float v)                    { return DirectX::XMLoadFloat(&v); }
		inline DirectX::XMVECTOR Load(const DirectX::XMFLOAT2& v) { return DirectX::XMLoadFloat2(&v); }
		inline DirectX::XMVECTOR Load(const DirectX::XMFLOAT3& v) { return DirectX::XMLoadFloat3(&v); }
		inline DirectX::XMVECTOR Load(const DirectX::XMFLOAT4& v) { return DirectX::XMLoadFloat4(&v); }

	}

}

namespace Shark::Math {

	template<typename T>
	T pow2(T val)
	{
		return pow(val, 2);
	}

	DirectX::XMFLOAT3 GetRotation(DirectX::XMMATRIX matrix);
	DirectX::XMFLOAT3 GetRotation(const DirectX::XMFLOAT4X4& matrix);


	constexpr float             ToRadians(float degrees)                    { return DirectX::XMConvertToRadians(degrees); }
	constexpr float             ToDegrees(float radians)                    { return DirectX::XMConvertToDegrees(radians); }
	constexpr DirectX::XMFLOAT2 ToRadians(const DirectX::XMFLOAT2& degrees) { return { ToRadians(degrees.x), ToRadians(degrees.y) }; }
	constexpr DirectX::XMFLOAT2 ToDegrees(const DirectX::XMFLOAT2& radians) { return { ToDegrees(radians.x), ToDegrees(radians.y) }; }
	constexpr DirectX::XMFLOAT3 ToRadians(const DirectX::XMFLOAT3& degrees) { return { ToRadians(degrees.x), ToRadians(degrees.y), ToRadians(degrees.z) }; }
	constexpr DirectX::XMFLOAT3 ToDegrees(const DirectX::XMFLOAT3& radians) { return { ToDegrees(radians.x), ToDegrees(radians.y), ToDegrees(radians.z) }; }
	constexpr DirectX::XMFLOAT4 ToRadians(const DirectX::XMFLOAT4& degrees) { return { ToRadians(degrees.x), ToRadians(degrees.y), ToRadians(degrees.z), ToRadians(degrees.w) }; }
	constexpr DirectX::XMFLOAT4 ToDegrees(const DirectX::XMFLOAT4& radians) { return { ToDegrees(radians.x), ToDegrees(radians.y), ToDegrees(radians.z), ToDegrees(radians.w) }; }

	DirectX::XMMATRIX ViewFromTransform(const DirectX::XMMATRIX& transform);

}
