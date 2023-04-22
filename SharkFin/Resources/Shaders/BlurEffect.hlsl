#pragma stage vertex

struct VSOUT
{
    float2 TexCoords : TexCoords;
    float4 Position : SV_Position;
};

VSOUT main(float2 pos : Position)
{
    VSOUT vsout;
    vsout.Position = float4(pos, 0.0f, 1.0f);
    vsout.TexCoords = float2((pos.x + 1.0f) * 0.5f, 1.0f - ((pos.y + 1.0f) * 0.5f));
    return vsout;
}

#pragma stage : pixel

Texture2D Frame;
SamplerState Sampler;

float4 main(float2 texCoords : TexCoords) : SV_Target0
{
    const float Radius = 5;
    const float Distance = 2 * Radius + 1;
    
    float width, height;
    Frame.GetDimensions(width, height);
    
    width = 1.0f / width;
    height = 1.0f / height;
    
    float4 color = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (float y = -Radius; y <= Radius; y += 1.0f)
    {
        for (float x = -Radius; x <= Radius; x += 1.0f)
        {
            color += Frame.Sample(Sampler, float2(texCoords.x + (x * width), texCoords.y + (y * height)));
        }
    }
    
    
    color = color / pow(Distance, 2);
    float4 originalColor = Frame.Sample(Sampler, texCoords);
    float4 newColor = max(originalColor, color);
    
    return color;
}
