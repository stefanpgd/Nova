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
ConstantBuffer<LightData> lights : register(b0, space1);

struct MaterialData
{
    bool hasAlbedo;
    bool hasNormal;
    bool hasMetallicRoughness;
    bool hasOclussion;
    
    uint OclussionChannel;
    uint MetallicChannel;
    uint RoughnessChannel;
};
ConstantBuffer<MaterialData> material : register(b0, space2);

Texture2D Diffuse : register(t0);
Texture2D Normal : register(t1);
Texture2D MetallicRoughness : register(t2);
Texture2D AmbientOcclusion : register(t3);

Texture2D Skydome : register(t0, space1);
Texture2D ShadowMap : register(t1, space1);

SamplerState LinearSampler : register(s0);

static float PI = 3.14159265;

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
    float3 incoming = normalize(IN.FragPosition - IN.CameraPosition);
    float3 sampleRay = reflect(incoming, normal);
    
    float3 n = normalize(sampleRay);
    float u = atan2(n.x, n.z) / (2.0 * PI) + 0.5;
    float v = n.y * 0.5 + 0.5;
    
    return Skydome.Sample(LinearSampler, float2(u, v)).rgb;
}

float D_GGX(float3 n, float3 h, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH = max(dot(n, h), 0.0);
    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = (NoH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float G_Shlick(float NoV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NoV;
    float denom = NoV * (1.0 - k) + k;

    return nom / denom;
}

float G_Smith(float3 n, float3 v, float3 l, float roughness)
{
    float NoV = max(dot(n, v), 0.0);
    float NoL = max(dot(n, l), 0.0);
    float ggx2 = G_Shlick(NoV, roughness);
    float ggx1 = G_Shlick(NoL, roughness);

    return ggx1 * ggx2;
}

float3 F_Shlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float F_Schlick90(float cosTheta, float f0, float f90)
{
    return f0 + (f90 - f0) * pow(1.0 - cosTheta, 5.0);
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness)
{
    float f90 = 0.5 + 2.0 * (roughness * roughness) * LoH * LoH;
    float lightScatter = F_Schlick90(NoL, 1.0, f90);
    float viewScatter = F_Schlick90(NoV, 1.0, f90);
    
    return lightScatter * viewScatter * (1.0 / PI);
}

float4 main(PixelIN IN) : SV_TARGET
{
    float3 albedo = float3(0.0, 0.0, 0.0);
    albedo = Diffuse.Sample(LinearSampler, IN.TexCoord).rgb;
    
    float metallic = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).b;
    float roughness = MetallicRoughness.Sample(LinearSampler, IN.TexCoord).g;
    float alpha = Diffuse.Sample(LinearSampler, IN.TexCoord).a;
    
    float3 tangentNormal = Normal.Sample(LinearSampler, IN.TexCoord).rgb * 2.0 - float3(1.0, 1.0, 1.0);
    
    float3 n = normalize(mul(tangentNormal, IN.TBN));
    float3 v = normalize(IN.CameraPosition - IN.FragPosition);
    
    // Determine if its a dielectric or a conductor based on the metallic parameter [0, 1]
    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, albedo, metallic);
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    
    float3 l = -normalize(float3(0.0, -0.738, -0.675));
    float3 h = normalize(v + l);
    
    // For now we only use our directional light //
    float directionalIntensity = 4.0;
    float3 radiance = float3(1.0, 1.0, 1.0) * directionalIntensity;
    
    // Cook-Torrance BRDF ( Fr )
    float D = D_GGX(n, h, roughness);
    float G = G_Smith(n, v, l, roughness);
    float3 F = F_Shlick(max(dot(h, v), 0.0), f0);
    
    float3 DGF = F * (D * G);
    float denom = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
    float3 Fr = DGF / denom;
    
    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
    float Fd = Fd_Burley(NoV, NoL, LoH, roughness);
    
    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;
    
    // Outgoing Irradiance
    Lo = (kD * albedo * Fd + Fr) * radiance * NoL;
    
    
    float3 ambient = 0.0;
    
    if(material.hasOclussion)
    {
        ambient = 0.3 * albedo * AmbientOcclusion.Sample(LinearSampler, IN.TexCoord).r;
    }
    else
    {
        ambient = 0.05 * albedo;
    }
    
    float3 color = ambient + Lo;
    
    float shadow = GetShadow(IN, n);
    float shadowStrength = min(shadow, 0.8);
    color *= (1.0 - shadowStrength);
    
    color = clamp(color, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
    
    //return float4(float3(ambient), alpha);
    
    
    float x = 0.0;
    
    if(material.hasOclussion)
    {
        x = 1.0;
    }
    
    return float4(float3(x, x, x), 1.0);
}