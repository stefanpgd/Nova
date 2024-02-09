struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D screenCapture : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 screen = screenCapture.Sample(LinearSampler, IN.TexCoord).rgb;
    return float4(screen, 1.0f);
}