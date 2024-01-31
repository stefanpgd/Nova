struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
    float3 FragPosition : FragPosition;
    float3 CameraPosition : CameraPosition;
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

Texture2D Diffuse : register(t0);
SamplerState LinearSampler : register(s0);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 albedo = Diffuse.Sample(LinearSampler, IN.Normal.xy).rgb;
    
    float3 total = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < lightData.activePointLights; i++)
    {
        float3 FragToLight = lightData.pointLights[i].Position - IN.FragPosition;
        float3 lightDir = normalize(FragToLight);
        
        float3 ambient = 0.04f * IN.Color;
        
        float diff = max(dot(IN.Normal, lightDir), 0.0);
        float3 diffuse = diff * IN.Color * float3(lightData.pointLights[i].Color.rgb);
        
        float3 specular = float3(0.0, 0.0, 0.0);
        
        if(diff > 0.0)
        {
            const float shininess = 24.0;
            float3 viewDirection = normalize(IN.CameraPosition - IN.FragPosition);
            
            float specularity = max(dot(viewDirection, reflect(-lightDir, IN.Normal)), 0.0);
            specularity = max(pow(specularity, shininess), 0.0);
            
            specular = specularity * float3(lightData.pointLights[i].Color.rgb);
        }
 
        float lightDistance = length(lightData.pointLights[i].Position - IN.FragPosition);
        
        const float constantFalloff = 1.0;
        const float linearFalloff = 0.1f;
        const float exponentialFalloff = 0.03f;
        
        float attenuation = 1.0f / (constantFalloff + linearFalloff * lightDistance + linearFalloff * lightDistance * lightDistance);
        float intensity = lightData.pointLights[i].Intensity * attenuation;
        
        float3 result = ambient + (diffuse + specular) * intensity;
        result = clamp(result, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
        
        total += result;
    }
    
    return float4(albedo, 1.0f);
}