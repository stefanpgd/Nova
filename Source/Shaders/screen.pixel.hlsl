struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D screenTexture : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float4 screen = screenTexture.Sample(LinearSampler, IN.TexCoord);
    
    if(screen.a < 0.1)
    {
        discard;
    }
    
    return float4(screen.rgb, 1.0f);
}