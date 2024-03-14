struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D screenTexture : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float4 sum = 0.0f;
    
    for (int x = -10; x < 10; x++)
    {
        for (int y = -10; y < 10; y++)
        {
            float2 offset = float2(x * 0.001, y * 0.001);
            sum += screenTexture.Sample(LinearSampler, IN.TexCoord + offset);
        }
    }
    
    sum /= 121.0;
    
    return float4(sum.rgb, 1.0f);
}