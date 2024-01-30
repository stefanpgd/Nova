struct PixelIN
{
    float3 Color : Color;
    float3 Normal : Normal;
    float3 ViewDirection : View;
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
    float3 stub;
};
ConstantBuffer<LightData> lightData : register(b0, space1);

float4 main(PixelIN IN) : SV_TARGET
{
    float3 normalC = (IN.Normal + float3(1.0, 1.0, 1.0)) * 0.5;
    
    float3 lightDir = normalize(float3(0.0f, -1.0f, 0.0f));
    float diff = max(dot(IN.Normal, -IN.ViewDirection), 0.0);
    
    float3 ambient = IN.Color * 0.4f;
    float3 diffuse = IN.Color * diff;
    
    float3 output = max(min(ambient + diffuse, float3(1.0, 1.0, 1.0)), float3(0.0f, 0.0f, 0.0f));
    
    int index = lightData.activePointLights;
    output = float3(lightData.pointLights[index].Color.rgb);
    
    return float4(float3(output), 1.0f);
}