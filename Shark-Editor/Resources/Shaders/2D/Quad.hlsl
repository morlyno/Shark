
struct VertexInput
{
    float3 Position : Position;
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TexIndex;
    float TilingFactor : TilingFactor;
    int ID : ID;
};

struct VertexToPixel
{
    float4 Color : Color;
    float2 TexCoord : TextureCoords;
    float TilingFactor : TilingFactor;
    int TexIndex : TexIndex;
    int ID : ID;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
};

#pragma stage : vertex

struct Camera
{
    matrix ViewProjection;
};

ConstantBuffer<Camera> u_Camera : register(b0, space1);

struct VertexOutput
{
    VertexToPixel V2P;
    float4 Position : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.Position = mul(u_Camera.ViewProjection, float4(input.Position, 1.0f));
    output.V2P.Color = input.Color;
    output.V2P.TexCoord = input.TexCoord;
    output.V2P.TilingFactor = input.TilingFactor;
    output.V2P.TexIndex = input.TexIndex;
    output.V2P.ID = input.ID;
    return output;
}

#pragma stage : Pixel
#pragma combine : u_Textures, u_Samplers;

Texture2D u_Textures[16] : register(t0, space0);
SamplerState u_Samplers[16] : register(s0, space0);

PixelOutput main(VertexToPixel input)
{
    PixelOutput output;
    output.ID = input.ID;
    
    switch (input.TexIndex)
    {
        case  0: output.Color = u_Textures[ 0].Sample(u_Samplers[ 0], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  1: output.Color = u_Textures[ 1].Sample(u_Samplers[ 1], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  2: output.Color = u_Textures[ 2].Sample(u_Samplers[ 2], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  3: output.Color = u_Textures[ 3].Sample(u_Samplers[ 3], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  4: output.Color = u_Textures[ 4].Sample(u_Samplers[ 4], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  5: output.Color = u_Textures[ 5].Sample(u_Samplers[ 5], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  6: output.Color = u_Textures[ 6].Sample(u_Samplers[ 6], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  7: output.Color = u_Textures[ 7].Sample(u_Samplers[ 7], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  8: output.Color = u_Textures[ 8].Sample(u_Samplers[ 8], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  9: output.Color = u_Textures[ 9].Sample(u_Samplers[ 9], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 10: output.Color = u_Textures[10].Sample(u_Samplers[10], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 11: output.Color = u_Textures[11].Sample(u_Samplers[11], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 12: output.Color = u_Textures[12].Sample(u_Samplers[12], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 13: output.Color = u_Textures[13].Sample(u_Samplers[13], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 14: output.Color = u_Textures[14].Sample(u_Samplers[14], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 15: output.Color = u_Textures[15].Sample(u_Samplers[15], input.TexCoord * input.TilingFactor) * input.Color; break;
        default: output.Color = input.Color; break;
    }

    output.Color.a = 1.0f;
    return output;
}