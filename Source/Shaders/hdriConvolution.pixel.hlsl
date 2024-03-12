struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D screenTexture : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float4 HDRI = screenTexture.Sample(LinearSampler, IN.TexCoord);
    
    return float4(HDRI.rgb, 1.0f);
}