#pragma once

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;
static const float Epsilon = 0.00001;

// Compute Van der Corput radical inverse
// See: http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 SampleHammersley(uint i, uint samples)
{
    float invSamples = 1.0 / float(samples);
    return float2(i * invSamples, radicalInverse_VdC(i));
}

float3 SampleHemisphere(float u1, float u2)
{
    const float u1p = sqrt(max(0.0, 1.0 - u1 * u1));
    return float3(cos(TwoPI * u2) * u1p, sin(TwoPI * u2) * u1p, u1);
}

void ComputeBasisVectors(const float3 N, out float3 S, out float3 T)
{
    T = cross(N, float3(0.0, 1.0, 0.0));
    T = lerp(cross(N, float3(1.0, 0.0, 0.0)), T, step(Epsilon, dot(T, T)));

    T = normalize(T);
    S = normalize(cross(N, T));
}

float3 TangentToWorld(const float3 v, const float3 N, const float3 S, const float3 T)
{
    return S * v.x + T * v.y + N * v.z;
}

float3 GetCubeMapTexCoord(uint3 dispatchID, float2 imageSize)
{
    float2 st = dispatchID.xy / imageSize;
    float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - (float2)1.0;
    
    float3 result;
    switch (dispatchID.z)
    {
        case 0:
            result = float3(1.0, uv.y, -uv.x);
            break;
        case 1:
            result = float3(-1.0, uv.y, uv.x);
            break;
        case 2:
            result = float3(uv.x, 1.0, -uv.y);
            break;
        case 3:
            result = float3(uv.x, -1.0, uv.y);
            break;
        case 4:
            result = float3(uv.x, uv.y, 1.0);
            break;
        case 5:
            result = float3(-uv.x, uv.y, -1.0);
            break;
    }
    return normalize(result);
}

// GGX/Trowbride-Reitz normal distribution
// Uses Disney's reparameterization of alpha = Roughness^2
float NDFGGX(float cosLh, float roughness)
{
    float alpha = roughness * roughness;
    float alphaSq = alpha * alpha;

    float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
    return alphaSq / (PI * denom * denom);
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// This returns normalized half-vector between Li & Lo.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html
float3 SampleGGX(float u1, float u2, float roughness)
{
    float alpha = roughness * roughness;

    float cosTheta = sqrt((1.0 - u2) / (1.0 + (alpha * alpha - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
    float phi = TwoPI * u1;

	// Convert to Cartesian upon return.
    return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}
