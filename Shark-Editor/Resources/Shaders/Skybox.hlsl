#pragma stage : vertex

struct Uniforms
{
    matrix SkyboxProjection;
};

ConstantBuffer<Uniforms> u_Uniforms : register(b0, space1);

struct VertexOutput
{
    float3 LocalPosition : LocalPosition;
    float4 Position : SV_Position;
};

VertexOutput main(float3 Position : Position)
{
    VertexOutput output;
    output.LocalPosition = Position;
    output.Position = mul(u_Uniforms.SkyboxProjection, float4(Position, 1.0f)).xyww;
    return output;
}

#pragma stage : pixel

#pragma combine : u_EnvironmentMap, u_Sampler

struct Settings
{
    float Lod;
    float Intensity;
    float P0, P1;
};

ConstantBuffer<Settings> u_Settings : register(b1, space1);
TextureCube u_EnvironmentMap : register(t0, space1);
SamplerState u_Sampler : register(s0, space1);

float4 main(float3 LocalPosition : LocalPosition) : SV_Target0
{
    float3 uv = normalize(LocalPosition);
    float3 color = u_EnvironmentMap.SampleLevel(u_Sampler, uv, u_Settings.Lod).rgb * u_Settings.Intensity;
    return float4(color, 1.0f);
}
