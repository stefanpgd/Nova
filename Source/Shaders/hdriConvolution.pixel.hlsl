struct PixelIN
{
    float2 TexCoord : TexCoord;
};

Texture2D hdriTexture : register(t0);
SamplerState LinearSampler : register(s0);

static float PI = 3.14159265359;

float4 main(PixelIN IN) : SV_TARGET
{
    // 0. Turn UV into normal direction
    float phi = IN.TexCoord.x * (2.0 * PI);
    float theta = (IN.TexCoord.y + 1.0) * PI;

    float3 normal = float3(cos(phi) * sin(theta), cos(theta), sin(phi) * sin(theta));
    
    // Grab texture coordinates, find out current angle values
    // Then use those base values, ADD the integral
    // Sum up results
    // divide accordingly, and save
    
    return float4(normal, 1.0f);
}