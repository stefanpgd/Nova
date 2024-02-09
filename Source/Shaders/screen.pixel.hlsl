struct PixelIN
{
    float2 TexCoord : TexCoord;
    float3 CameraFront : CameraFront;
    float3 FragPosition : FragPosition;
};

Texture2D screenCapture : register(t0);
Texture2D skydome : register(t1);
SamplerState LinearSampler : register(s0);

float3 GetSkydomeColor(PixelIN IN)
{
    float pi = 3.14159265;
    
    float3 sampleRay = normalize(IN.CameraFront + IN.FragPosition * 0.75);
    
    float theta = cos(sampleRay.y);
    float phi = atan2(sampleRay.z, sampleRay.x) + pi;
   
    float3 n = normalize(sampleRay);
    float u = atan2(n.x, n.z) / (2.0 * pi) + 0.5;
    float v = n.y * 0.5 + 0.5;
    
    return skydome.Sample(LinearSampler, float2(u, v)).rgb;
}

float4 main(PixelIN IN) : SV_TARGET
{
    float4 screen = screenCapture.Sample(LinearSampler, IN.TexCoord);
    
    if (screen.a < 0.1)
    {
        float3 sky = GetSkydomeColor(IN);
        return float4(sky, 1.0f);
    }
    
    return float4(screen.rgb, 1.0f);
}