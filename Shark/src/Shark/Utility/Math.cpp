#include "skpch.h"
#include "Math.h"

using namespace DirectX;

namespace Shark {

    XMFLOAT3 operator+(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z }; }
    XMFLOAT3& operator+=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs = lhs + rhs; }
    XMFLOAT3 operator-(const XMFLOAT3& lhs, const XMFLOAT3& rhs) { return { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z }; }
    XMFLOAT3& operator-=(XMFLOAT3& lhs, const XMFLOAT3& rhs) { return lhs = lhs - rhs; }

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

}
