struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
    float3 ViewDirection : View;
};

struct LightData
{
    float3 direction;
    float stub[61];
};
ConstantBuffer<LightData> lightData : register(b0, space1);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 normalC = (IN.Normal + float3(1.0, 1.0, 1.0)) * 0.5;
    
    float3 lightDir = normalize(float3(0.0f, -1.0f, 0.0f));
    float diff = max(dot(IN.Normal, -lightData.direction), 0.0);
    
    float3 ambient = IN.Color * 0.4f;
    float3 diffuse = IN.Color * diff;
    
    float3 output = max(min(ambient + diffuse, float3(1.0, 1.0, 1.0)), float3(0.0f, 0.0f, 0.0f));
    
    float gamma = 1.0 / 2.4;
    output = pow(output, float3(gamma, gamma, gamma));
    
    return float4(float3(diff, diff, diff), 1.0f);
}