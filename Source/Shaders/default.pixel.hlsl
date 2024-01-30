struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
    float3 FragPosition : FragPosition;
};

struct PointLight
{
    float3 Position;
    float Intensity;
    float4 Color;
};

struct LightData
{
    PointLight pointLights[15];
    int activePointLights;
};
ConstantBuffer<LightData> lightData : register(b0, space1);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 total = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < lightData.activePointLights; i++)
    {
        float3 FragToLight = lightData.pointLights[i].Position - IN.FragPosition;
        float diff = dot(IN.Normal, normalize(FragToLight));
        float3 diffuse = diff * IN.Color * float3(lightData.pointLights[i].Color.rgb);
        
        total += diffuse;
    }
    
    return float4(total, 1.0f);
}