struct PixelIN
{
    float3 Color : Color;
    float3x3 TBN : TBN;
    float3 FragPosition : FragPosition;
    float3 CameraPosition : CameraPosition;
    float2 TexCoord : TexCoord;
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
Texture2D Normal : register(t1);
Texture2D MetallicRoughness : register(t2);
Texture2D AmbientOcclusion : register(t3);

Texture2D Skydome : register(t0, space1);

SamplerState LinearSampler : register(s0);

float3 GetSkydomeColor(PixelIN IN, float3 normal)
{
    float pi = 3.14159265;
    
    float3 incoming = normalize(IN.FragPosition - IN.CameraPosition);
    float3 sampleRay = reflect(incoming, normal);
    
    float3 n = normalize(sampleRay);
    float u = atan2(n.x, n.z) / (2.0 * pi) + 0.5;
    float v = n.y * 0.5 + 0.5;
    
    return Skydome.Sample(LinearSampler, float2(u, v)).rgb;
}

float4 main(PixelIN IN) : SV_TARGET
{
    float3 albedo = Diffuse.Sample(LinearSampler, IN.TexCoord).rgb;
    float3 tangentNormal = Normal.Sample(LinearSampler, IN.TexCoord).rgb * 2.0 - float3(1.0, 1.0, 1.0);
    float3 normal = normalize(mul(tangentNormal, IN.TBN));
   
    if (!any(albedo) && !any(normal))
    {
        albedo = IN.Color;
    }
    
    float3 total = float3(0.0f, 0.0f, 0.0f);
    
    for(int i = 0; i < lightData.activePointLights; i++)
    {
        float3 color = albedo;
        
        float3 FragToLight = lightData.pointLights[i].Position - IN.FragPosition;
        float3 lightDir = normalize(FragToLight);
        
        float diff = max(dot(normal, lightDir), 0.0);
        float3 diffuse = diff * color * float3(lightData.pointLights[i].Color.rgb);
        
        float3 specular = float3(0.0, 0.0, 0.0);
        
        if(diff > 0.0)
        {
            const float shininess = 124.0;
            float3 viewDirection = normalize(IN.CameraPosition - IN.FragPosition);
            float specularity = max(dot(viewDirection, reflect(-lightDir, normal)), 0.0);
            specularity = max(pow(specularity, shininess), 0.0);
            
            float metallic = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).g;
            float roughness = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).b;
            
            specular = (specularity * metallic * (1.0 - roughness)) * GetSkydomeColor(IN, normal);
        }
    
        float lightDistance = length(lightData.pointLights[i].Position - IN.FragPosition);
        
        const float constantFalloff = 1.0;
        const float linearFalloff = 0.1f;
        const float exponentialFalloff = 0.03f;
        
        float attenuation = 1.0f / (constantFalloff + linearFalloff * lightDistance + linearFalloff * lightDistance * lightDistance);
        float intensity = lightData.pointLights[i].Intensity * attenuation;
        
        float3 result = (diffuse + specular) * intensity;
        result = clamp(result, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
        
        total += result;
    }
    
    total = clamp(total, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
    
    float occlusion = AmbientOcclusion.Sample(LinearSampler, IN.TexCoord).r;
    occlusion = 1.0; // temp due to the fact that not many models use occlusion...
    
    return float4(total * occlusion, 1.0f);
}