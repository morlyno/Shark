#pragma stage : Vertex

struct Camera
{
    matrix ViewProjection;
    float3 Position;
    float Padding;
};

struct PushConstant
{
    matrix Transform;
};

[[vk::push_constant]]
ConstantBuffer<PushConstant> u_Push : register(b0, space0);
ConstantBuffer<Camera> u_Camera : register(b1, space1);

struct VertexInput
{
    float3 Position : Position;
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float3 Bitangent : Bitangent;
    float2 Texcoord : Texcoord;
};

struct VertexOutput
{
    float3 Normal : Normal;
    float2 Texcoord : Texcoord;
    float4 Position : SV_POSITION;
};

VertexOutput main(VertexInput Input)
{
    VertexOutput Output;
    
    float4 pos = mul(u_Push.Transform, float4(Input.Position, 1.0f));
    Output.Position = mul(u_Camera.ViewProjection, pos);
    Output.Normal = normalize(mul((float3x3) u_Push.Transform, Input.Normal));
    Output.Texcoord = Input.Texcoord;
    
    return Output;
}

#pragma stage : Pixel

#pragma combine : u_Texture, u_Sampler

Texture2D u_Texture : register(t0, space0);
SamplerState u_Sampler : register(s0, space0);

struct PixelInput
{
    float3 Normal : Normal;
    float2 Texcoord : Texcoord;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
};

float4 ToLinear(float4 sRGB)
{
    bool4 cutoff = sRGB < (float4) 0.04045;
    float4 higher = pow((sRGB + 0.055) / 1.055, 2.4);
    float4 lower = sRGB / 12.92;
    return lerp(higher, lower, cutoff);
}

PixelOutput main(PixelInput Input)
{
    PixelOutput Output;
    Output.Color = float4(u_Texture.Sample(u_Sampler, Input.Texcoord).xyz, 1.0f);
    Output.ID = -1;
    return Output;
}
