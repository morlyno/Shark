
void BoundsCheck(inout float2 xy, float2 uv)
{
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
      xy = float2(1000.0f, 1000.0f);
}

float ScreenDistance(float2 v, float2 texelSize)
{
    float ratio = texelSize.x / texelSize.y;
    v.x /= ratio;
    return dot(v, v);
}
