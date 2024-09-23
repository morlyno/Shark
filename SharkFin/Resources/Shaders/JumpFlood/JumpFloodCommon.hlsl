
struct Camera
{
    matrix ViewProjection;
    float3 Position;
    float Padding;
};

struct Uniforms
{
    float4 TexelSize;
    float4 OutlineColor;
    float2 FrameBufferSize;
    float OutlineWidth;
    
    float P0;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;
[[vk::binding(1, 1)]] ConstantBuffer<Uniforms> u_Uniforms;

[[vk::binding(0, 0)]] uniform Texture2D u_MainTexture;

// just inside the precision of a R16G16_SNorm to keep encoded range 1.0 >= and > -1.0
#define SNORM16_MAX_FLOAT_MINUS_EPSILON ((float)(32768-2) / (float)(32768-1))
#define FLOOD_ENCODE_OFFSET float2(1.0, SNORM16_MAX_FLOAT_MINUS_EPSILON)
#define FLOOD_ENCODE_SCALE float2(2.0, 1.0 + SNORM16_MAX_FLOAT_MINUS_EPSILON)

#define FLOOD_NULL_POS -1.0
#define FLOOD_NULL_POS_FLOAT2 float2(FLOOD_NULL_POS, FLOOD_NULL_POS)
