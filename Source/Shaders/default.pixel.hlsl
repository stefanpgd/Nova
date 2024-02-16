struct PixelIN
{
    float3 Color : Color;
    float3x3 TBN : TBN;
    float3 FragPosition : FragPosition;
    float4 FragLight : FragLight;
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

Texture2D Skydome : register(t0, space1);
Texture2D ShadowMap : register(t1, space1);

SamplerState LinearSampler : register(s0);

float GetShadow(PixelIN IN, float3 normal)
{
    int width, height, levels;
    ShadowMap.GetDimensions(0, width, height, levels);
    float2 texelSize = float2(1.0, 1.0) / float2(width, height);
    
    float3 projectCoords = IN.FragLight.xyz / IN.FragLight.w;
    float2 uv = 0.5f * projectCoords.xy + 0.5f;
    uv.y = 1.0f - uv.y;
    
    float closestDepth = ShadowMap.Sample(LinearSampler, uv).r;
    float currentDepth = projectCoords.z;
    
    float3 lightDir = normalize(float3(0.675, -0.738, 0.0f));
    float bias = max(0.001 * (1.0 - dot(normal, lightDir)), 0.0001);
    
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = ShadowMap.Sample(LinearSampler, uv.xy + float2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

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
    float alpha = Diffuse.Sample(LinearSampler, IN.TexCoord).a;
    float3 tangentNormal = Normal.Sample(LinearSampler, IN.TexCoord).rgb * 2.0 - float3(1.0, 1.0, 1.0);
    float3 normal = normalize(mul(tangentNormal, IN.TBN));
    
    if (!any(albedo) && !any(normal))
    {
        albedo = IN.Color;
    }
    
    float3 lightDir = normalize(float3(0.675, -0.738, 0.0f));
    
    float3 total = float3(0.0f, 0.0f, 0.0f);
    float3 diffuse = float3(0.0, 0.0, 0.0);
    float3 specular = float3(0.0, 0.0, 0.0);
    
    float diff = max(dot(normal, -lightDir), 0.0);
    diffuse = diff * albedo;
    
    if (diff > 0.0)
    {
        const float shininess = 124.0;
        float3 viewDirection = normalize(IN.CameraPosition - IN.FragPosition);
        float specularity = max(dot(viewDirection, reflect(lightDir, normal)), 0.0);
        specularity = max(pow(specularity, shininess), 0.0);
            
        float metallic = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).g;
        float roughness = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).b;
            
        specular = (specularity * metallic * (1.0 - roughness)) * GetSkydomeColor(IN, normal);
    }
    
    float shadow = GetShadow(IN, normal);
    total = 0.15 * albedo + (1.0 - shadow) * (diffuse + specular);
   
    return float4(total, alpha);
}