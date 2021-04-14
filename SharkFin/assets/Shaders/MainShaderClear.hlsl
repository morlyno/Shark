#type Vertex

cbuffer ClearData : register(b0)
{
    float4 ClearColor = float4(0.1f, 0.1f, 0.1f, 1.0f);
    int ClearID = -1;
};

struct Input
{
    int VertexIndex : VertexIndex;
};

struct Output
{
    float4 ClearColor : ClearColor;
    int ClearID : ClearID;
    float4 Pos : SV_Position;
};

float2 g_Vertices[4] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };

Output main(Input input)
{
    Output output;
    output.Pos = float4(g_Vertices[input.VertexIndex], 0.0f, 1.0f);
    output.ClearColor = ClearColor;
    output.ClearID = ClearID;
    return output;
}

#type Pixel

struct Input
{
    float4 ClearColor : ClearColor;
    int ClearID : ClearID;
};

struct Output
{
    float4 ClearColor : SV_Target0;
    int ClearID : SV_Target1;
};

Output main(Input input)
{
    Output output;
    output.ClearColor = input.ClearColor;
    output.ClearID = input.ClearID;
    return output;
}