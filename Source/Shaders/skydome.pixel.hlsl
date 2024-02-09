struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D skydome : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 sky = skydome.Sample(LinearSampler, IN.TexCoord).rgb;
    return float4(sky, 1.0f);
}