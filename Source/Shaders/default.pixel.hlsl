struct PixelIN
{
    float3 Color : Color0;
};

float4 main(PixelIN IN) : SV_TARGET
{
    return float4(IN.Color, 1.0f);
}