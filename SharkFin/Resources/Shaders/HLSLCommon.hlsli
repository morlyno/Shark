
float2 GetTexelSize(Texture2D texture)
{
    float2 texelSize;
    texture.GetDimensions(texelSize.x, texelSize.y);
    return 1.0f / texelSize;
}
