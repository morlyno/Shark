#pragma stage : vertex

struct VertexInput
{
    float3 Position : Position;
    float2 TexCoord : TexCoord;
};

struct VertexOutput
{
    float2 TexCoord : TexCoord;
    float4 Position : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.TexCoord = input.TexCoord;
    output.Position = float4(input.Position, 1.0f);
    return output;
}

#pragma stage : pixel

#include "Core/Samplers.hlslh"

struct Settings
{
    bool Tonemap;
    bool GammaCorrect;
    float Exposure;
    float P0;
};

ConstantBuffer<Settings> u_Settings : register(b0, space1);
Texture2D u_Input : register(t0, space1);

// ACES Tonemap
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
float3 Tonemap(float3 color)
{
    float3x3 m1 = float3x3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777
	);
    float3x3 m2 = float3x3(
		 1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602
	);

    float3 v = mul(m1, color);
    float3 a = v * (v + 0.0245786) - 0.000090537;
    float3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return clamp(mul(m2, a / b), 0.0, 1.0);
}

float3 GammaCorrect(float3 color, float gamma)
{
    return pow(color, (float3)(1.0f / gamma));
}

float4 main(float2 TexCoord : TexCoord) : SV_Target
{
    const float gamma = 2.2;
    const float pureWhite = 1.0;
    float samplesScale = 0.5;

    float3 color = u_Input.Sample(u_LinearClamp, TexCoord).rgb;

    color *= u_Settings.Exposure;

    if (u_Settings.Tonemap)
    {
        color = Tonemap(color);
    }

    if (u_Settings.GammaCorrect)
    {
        color = GammaCorrect(color, gamma);
    }

    return float4(color, 1.0f);
}
