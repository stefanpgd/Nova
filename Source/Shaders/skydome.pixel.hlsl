struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D skydome : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float2 uv = float2(IN.TexCoord.x + 0.77, IN.TexCoord.y);
    float3 sky = skydome.Sample(LinearSampler, uv).rgb;
    
    float g = 1.0 / 2.2;
    float3 color = pow(abs(sky), float3(g, g, g));
    
    return float4(color, 1.0f);
}