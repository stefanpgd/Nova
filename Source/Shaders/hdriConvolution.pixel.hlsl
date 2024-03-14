struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D hdriTexture : register(t0);
SamplerState LinearSampler : register(s0);

static float PI = 3.14159265359;

float2 GetUVFromVector(float3 n)
{
    n = normalize(n);
    float phi = atan2(n.z, n.x) + PI;
    float theta = acos(n.y);
    
    float u = abs(phi / (2 * PI));
    float v = abs(1.0 - (theta / PI));
    
    return float2(u, v);
}

float3 SphericalToCartesian(float phi, float theta)
{
    return float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
}

float4 main(PixelIN IN) : SV_TARGET
{
    // 0. Turn UV into normal direction
    float basePhi = IN.TexCoord.x * (2.0 * PI);
    float baseTheta = (IN.TexCoord.y + 1.0) * PI;

    float3 normal = normalize(SphericalToCartesian(basePhi, baseTheta));
    float rightTheta = baseTheta + (PI * 0.5);
    float3 right = normalize(SphericalToCartesian(basePhi, rightTheta));
    
    float3 up = normalize(cross(normal, right));
    
    float3 irradiance = float3(0.0, 0.0, 0.0);
    float sampleDelta = 0.02; // lower the delta, the more accurate but expensive
    float sampleCount = 0;
    float maxIrradiance = 2500.0;
    
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            float3 tangentSample = SphericalToCartesian(phi, theta);
            float3 sampleDir = tangentSample.x * right + tangentSample.y * normal + tangentSample.z * up;
            float2 uv = GetUVFromVector(sampleDir);
            
            float3 sample = hdriTexture.Sample(LinearSampler, uv).rgb;
            sample = clamp(sample, float3(0.0, 0.0, 0.0), float3(maxIrradiance, maxIrradiance, maxIrradiance));
            
            irradiance += sample * cos(theta) * sin(theta);
            sampleCount += 1.0;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(sampleCount));
    //irradiance = irradiance / (irradiance + float3(1.0, 1.0, 1.0));
    
    //float g = 1.0 / 2.2;
    //irradiance = pow(abs(irradiance), float3(g, g, g));
    
    // Grab texture coordinates, find out current angle values
    // Then use those base values, ADD the integral
    // Sum up results
    // divide accordingly, and save
    
    return float4(irradiance, 1.0f);
}