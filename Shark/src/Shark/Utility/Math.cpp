#include "skpch.h"
#include "Math.h"

using namespace DirectX;

namespace Shark {

    XMFLOAT3 operator+(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z }; }
    XMFLOAT3& operator+=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs = lhs + rhs; }
    XMFLOAT3 operator-(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }
    XMFLOAT3& operator-=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs = lhs - rhs; }

    XMVECTOR    XM_CALLCONV     operator+ (FXMVECTOR V) { return V; }
    XMVECTOR    XM_CALLCONV     operator- (FXMVECTOR V) { return DirectX::XMVectorNegate(V); }

    XMVECTOR& XM_CALLCONV     operator+= (XMVECTOR& V1, FXMVECTOR V2) { V1 = DirectX::XMVectorAdd(V1, V2); return V1; }
    XMVECTOR& XM_CALLCONV     operator-= (XMVECTOR& V1, FXMVECTOR V2) { V1 = DirectX::XMVectorSubtract(V1, V2); return V1; }
    XMVECTOR& XM_CALLCONV     operator*= (XMVECTOR& V1, FXMVECTOR V2) { V1 = DirectX::XMVectorMultiply(V1, V2); return V1; }
    XMVECTOR& XM_CALLCONV     operator/= (XMVECTOR& V1, FXMVECTOR V2) { V1 = DirectX::XMVectorDivide(V1, V2); return V1; }

    XMVECTOR& operator*= (XMVECTOR& V, float S) { V = DirectX::XMVectorScale(V, S); return V; }
    XMVECTOR& operator/= (XMVECTOR& V, float S) { XMVECTOR vS = DirectX::XMVectorReplicate(S); V = DirectX::XMVectorDivide(V, vS); return V; }

    XMVECTOR    XM_CALLCONV     operator+ (FXMVECTOR V1, FXMVECTOR V2) { return DirectX::XMVectorAdd(V1, V2); }
    XMVECTOR    XM_CALLCONV     operator- (FXMVECTOR V1, FXMVECTOR V2) { return DirectX::XMVectorSubtract(V1, V2); }
    XMVECTOR    XM_CALLCONV     operator* (FXMVECTOR V1, FXMVECTOR V2) { return DirectX::XMVectorMultiply(V1, V2); }
    XMVECTOR    XM_CALLCONV     operator/ (FXMVECTOR V1, FXMVECTOR V2) { return DirectX::XMVectorDivide(V1, V2); }
    XMVECTOR    XM_CALLCONV     operator* (FXMVECTOR V, float S) { return DirectX::XMVectorScale(V, S); }
    XMVECTOR    XM_CALLCONV     operator* (float S, FXMVECTOR V) { XMVECTOR vS = DirectX::XMVectorReplicate(S); return DirectX::XMVectorDivide(V, vS); }
    XMVECTOR    XM_CALLCONV     operator/ (FXMVECTOR V, float S) { return DirectX::XMVectorScale(V, S); }

}

namespace Shark::Math {


    XMFLOAT3 GetRotation(XMMATRIX matrix)
    {
        XMMATRIX mat;
        mat.r[0] = XMVector4Normalize(matrix.r[0]);
        mat.r[1] = XMVector4Normalize(matrix.r[1]);
        mat.r[2] = XMVector4Normalize(matrix.r[2]);

        float pitch = DirectX::XMScalarASin(-XMVectorGetY(mat.r[2]));
        
        XMVECTOR from(XMVectorSet(XMVectorGetY(mat.r[0]), XMVectorGetX(mat.r[2]), 0.0f, 0.0f));
        XMVECTOR to(XMVectorSet(XMVectorGetY(mat.r[1]), XMVectorGetZ(mat.r[2]), 0.0f, 0.0f));
        XMVECTOR res(XMVectorATan2(from, to));

        float roll = XMVectorGetX(res);
        float yaw = XMVectorGetY(res);

        return XMFLOAT3(pitch, yaw, roll);
    }

    XMFLOAT3 GetRotation(const XMFLOAT4X4& matrix)
    {
        return GetRotation(XMLoadFloat4x4(&matrix));
    }

	float ToRadians(float degrees)
	{
        return DirectX::XMConvertToRadians(degrees);
	}

	float ToDegrees(float radians)
	{
        return DirectX::XMConvertToDegrees(radians);
	}

}
